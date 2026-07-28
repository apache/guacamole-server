#!/usr/bin/env python3
"""
Manual end-to-end test client for Guacamole terminal text-output mode.

Unlike text-output-tunnel-smoke.py (which drives the full web-app + WebSocket
tunnel), this script speaks the Guacamole protocol *directly to guacd* over its
TCP socket. It performs the handshake for a terminal protocol (ssh / telnet /
kubernetes), then captures the outbound "STDOUT" pipe stream opened by
text-output mode and decodes its blobs, verifying that the raw remote byte
stream -- including ANSI/escape sequences -- arrives verbatim.

It also supports negative validation (--expect-no-pipe): confirming that no
STDOUT pipe is opened when text-output is off or is gated by disable-copy.

guacd host/port default to 127.0.0.1:4822 (override with GUACD_HOST/GUACD_PORT).
Requires only the Python standard library.

Usage:
    text-output-guacd-e2e.py <protocol> '<json-params>' [options]

Options:
    --secs N            How long to read the session stream (default 8).
    --expect SUBSTR     Require SUBSTR to appear in the captured raw bytes.
    --expect-absent S   Require S to NOT appear (e.g. to show dropped output
                        under backpressure when combined with --no-ack).
    --expect-no-pipe    Pass only if NO STDOUT pipe is opened (negative test).
    --max-graphics N    Require graphical-instruction bytes (img/blob/rect/...)
                        to stay at or below N (e.g. to show raw mode suppresses
                        the graphical stream).
    --no-ack            Do not acknowledge blobs, to exercise flow control.

Example:
    text-output-guacd-e2e.py ssh \\
        '{"hostname":"127.0.0.1","port":"22","username":"u","password":"p",
          "text-output":"true","command":"printf hi"}' --expect hi
"""
import socket
import sys
import os
import json
import base64
import time
import re

HOST = os.environ.get("GUACD_HOST", "127.0.0.1")
PORT = int(os.environ.get("GUACD_PORT", "4822"))


def enc(*elems):
    """Encode a Guacamole protocol instruction from its elements."""
    return ",".join("%d.%s" % (len(e), e) for e in elems) + ";"


def enc_len(elems):
    """Exact on-wire byte length of an instruction, from its (bytes) elements."""
    n = sum(len(str(len(e))) + 1 + len(e) for e in elems)  # "LEN" + "." + value
    return n + (len(elems) - 1) + 1                          # commas + ';'


class Parser:
    """Incremental parser for the Guacamole instruction stream."""

    def __init__(self):
        self.buf = b""

    def feed(self, data):
        self.buf += data

    def __iter__(self):
        return self

    def __next__(self):
        b = self.buf
        i = 0
        elems = []
        while True:
            dot = b.find(b".", i)
            if dot < 0:
                raise StopIteration
            try:
                length = int(b[i:dot])
            except ValueError:
                raise StopIteration
            start = dot + 1
            end = start + length
            if end >= len(b):
                raise StopIteration
            elems.append(b[start:end])
            sep = b[end:end + 1]
            i = end + 1
            if sep == b";":
                self.buf = b[i:]
                return elems
            if sep == b",":
                continue
            raise StopIteration


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)

    proto = sys.argv[1]
    params = json.loads(sys.argv[2])
    secs = 8.0
    expect = None
    expect_absent = None
    expect_no_pipe = False
    no_ack = False
    max_graphics = None
    a = sys.argv[3:]
    for j, v in enumerate(a):
        if v == "--secs":
            secs = float(a[j + 1])
        elif v == "--expect":
            expect = a[j + 1]
        elif v == "--expect-absent":
            expect_absent = a[j + 1]
        elif v == "--expect-no-pipe":
            expect_no_pipe = True
        elif v == "--max-graphics":
            max_graphics = int(a[j + 1])
        elif v == "--no-ack":
            no_ack = True

    s = socket.create_connection((HOST, PORT), timeout=10)
    s.sendall(enc("select", proto).encode())

    p = Parser()
    names = None
    version = "VERSION_1_5_0"

    # Handshake: read the "args" instruction guacd sends. Its first element is
    # the protocol version; the rest are the declared parameter names.
    s.settimeout(10)
    while names is None:
        data = s.recv(65536)
        if not data:
            print("EOF during handshake")
            sys.exit(2)
        p.feed(data)
        for inst in p:
            if inst[0] == b"args":
                els = [e.decode(errors="replace") for e in inst[1:]]
                if els and els[0].startswith("VERSION"):
                    version = els[0]
                names = els[1:]
    print("[handshake] version=%s  %d declared args" % (version, len(names)))

    # Minimal client capabilities, then connect. The "connect" instruction must
    # carry num_args + 1 elements: the client version echo followed by one value
    # per declared parameter name (positionally).
    s.sendall(enc("size", "1024", "768", "96").encode())
    s.sendall(enc("audio").encode())
    s.sendall(enc("video").encode())
    s.sendall(enc("image").encode())
    values = [params.get(n, "") for n in names]
    s.sendall(enc("connect", version, *values).encode())
    print("[connect] sent %d values; set: %s"
          % (len(values), {n: params[n] for n in names if n in params}))

    stdout_idx = None
    captured = bytearray()
    pipe_seen = False
    graphics_bytes = 0
    control_ops = ("sync", "ready", "args", "nop", "disconnect", "error")
    deadline = time.time() + secs
    s.settimeout(1.0)
    while time.time() < deadline:
        try:
            data = s.recv(65536)
        except socket.timeout:
            continue
        if not data:
            print("[stream] guacd closed connection")
            break
        p.feed(data)
        for inst in p:
            op = inst[0].decode(errors="replace")
            # Graphics accounting: anything that is neither the STDOUT text pipe
            # nor a control/handshake instruction is graphical output.
            is_stdout = (op == "pipe" and len(inst) > 3 and inst[3] == b"STDOUT") \
                or (op in ("blob", "end") and stdout_idx is not None
                    and inst[1].decode() == stdout_idx)
            if op not in control_ops and not is_stdout:
                graphics_bytes += enc_len(inst)
            if op == "sync":
                # Echo sync to keep guacd's frame loop (and pipe flushing) alive.
                ts = inst[1].decode() if len(inst) > 1 else "0"
                s.sendall(enc("sync", ts).encode())
            elif op == "pipe":
                idx, mimetype, name = inst[1].decode(), inst[2].decode(), inst[3].decode()
                print("[pipe] index=%s mimetype=%s name=%s" % (idx, mimetype, name))
                if name == "STDOUT":
                    stdout_idx, pipe_seen = idx, True
            elif op == "blob" and stdout_idx is not None and inst[1].decode() == stdout_idx:
                captured += base64.b64decode(inst[2])
                # Acknowledge the blob so guacd's flow control lets more
                # through (unless deliberately withheld to test backpressure).
                if not no_ack:
                    s.sendall(enc("ack", stdout_idx, "", "0").encode())
            elif op == "error":
                print("[error] %s" % b",".join(inst[1:]).decode(errors="replace"))
            elif op == "disconnect":
                print("[stream] disconnect")
                deadline = 0

    print("\n===== RESULT =====")
    print("STDOUT pipe opened : %s" % pipe_seen)
    print("raw bytes captured : %d" % len(captured))
    print("contains ESC (0x1b): %s" % (b"\x1b" in captured))
    snippet = re.sub(r"[\x00-\x08\x0e-\x1f]", ".",
                     captured.decode("utf-8", errors="replace"))[:400]
    print("snippet            : %r" % snippet)
    print("graphics bytes     : %d" % graphics_bytes)

    if expect_no_pipe:
        ok = not pipe_seen
        print("expect NO pipe     : %s" % ok)
    else:
        ok = pipe_seen and len(captured) > 0
        if expect is not None:
            found = expect.encode() in captured
            print("expect %r        : %s" % (expect, found))
            ok = ok and found
        if expect_absent is not None:
            absent = expect_absent.encode() not in captured
            print("expect absent %r : %s" % (expect_absent, absent))
            ok = ok and absent
    if max_graphics is not None:
        within = graphics_bytes <= max_graphics
        print("graphics <= %-6d : %s" % (max_graphics, within))
        ok = ok and within
    print("VERDICT            : %s" % ("PASS" if ok else "FAIL"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
