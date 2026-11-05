#!/usr/bin/env python3
"""
Mock Kubernetes API "exec" endpoint for testing guacd's kubernetes protocol.

guacd's kubernetes client connects to the Kubernetes API pod-exec endpoint over
a WebSocket using subprotocol "v4.channel.k8s.io", and reads binary frames whose
first byte is the stream channel (1 = stdout, 2 = stderr). This server mimics
that endpoint and emits channel-1 frames containing raw ANSI, so text-output
mode can be validated end to end without a real cluster.

Point a guacd kubernetes connection at this server with use-ssl=false, e.g.
hostname=127.0.0.1 port=8091 namespace=default pod=testpod exec-command=/bin/sh.

Requires the third-party "websockets" Python package.

Usage: text-output-k8s-exec-mock.py [port] [lines] [pad]  (defaults: 8091, 20, 0)
"""
import asyncio
import sys

try:
    import websockets
except ImportError as exc:  # pragma: no cover - manual dependency check
    raise SystemExit('Missing dependency: install the "websockets" Python package.') from exc

SUBPROTO = "v4.channel.k8s.io"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8091
LINES = int(sys.argv[2]) if len(sys.argv) > 2 else 20

# Optional per-line padding, in bytes. Used to push a modest number of lines
# past guacd's unacknowledged-backlog byte bound without needing a line count
# so large that the per-line delay dominates the test's runtime.
PAD = int(sys.argv[3]) if len(sys.argv) > 3 else 0


def frame(text):
    """Wrap text as a Kubernetes exec stdout (channel 1) binary frame."""
    return b"\x01" + text.encode()


async def handler(ws, *args):
    print("[k8s-mock] client connected, subproto=%r path=%s"
          % (getattr(ws, "subprotocol", None), getattr(ws, "path", "?")), flush=True)
    try:
        await ws.send(frame("\x1b[36mK8S-MOCK-EXEC ready\x1b[0m\r\n"))
        for i in range(LINES):
            await ws.send(frame("\x1b[32mK8S-TEXT-OUTPUT line %02d\x1b[0m%s\r\n"
                                % (i, "P" * PAD)))
            await asyncio.sleep(0.1)
    except Exception as exc:  # noqa: BLE001 - report and exit the handler
        print("[k8s-mock] handler ended: %r" % exc, flush=True)


async def main():
    async with websockets.serve(handler, "127.0.0.1", PORT, subprotocols=[SUBPROTO]):
        print("[k8s-mock] listening ws://127.0.0.1:%d (subproto %s)" % (PORT, SUBPROTO), flush=True)
        await asyncio.Future()


if __name__ == "__main__":
    asyncio.run(main())
