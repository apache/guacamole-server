# text-output end-to-end manual tests

These manual tests validate the opt-in terminal **text-output** mode (issue #3)
directly against a running `guacd`, for all three terminal protocols (SSH,
telnet, Kubernetes). They complement `text-output-tunnel-smoke.py`, which
instead drives the full web application over the WebSocket tunnel.

## Files

| File | Purpose |
| --- | --- |
| `text-output-guacd-e2e.py` | A minimal Guacamole-protocol client that speaks directly to `guacd`, performs the handshake for a protocol, and captures/decodes the outbound `STDOUT` pipe opened by text-output mode. Supports `--expect <substr>` and `--expect-no-pipe`. Stdlib only. |
| `text-output-k8s-exec-mock.py` | A mock Kubernetes API pod-`exec` WebSocket endpoint (subprotocol `v4.channel.k8s.io`) that emits raw ANSI on channel 1, so the kubernetes protocol can be tested without a real cluster. Needs the `websockets` package. |
| `text-output-e2e.sh` | Orchestrates the whole suite: positive path (raw ANSI is teed) and negative path (no pipe when text-output is off or gated by `disable-copy`) for all three protocols. |

## What it checks

Positive path (per protocol): text-output opens a `STDOUT` pipe
(`application/octet-stream`) and the exact remote byte stream — including ANSI
escape sequences — is delivered verbatim.

Negative path: no `STDOUT` pipe is opened when `text-output` is unset, and none
is opened when `text-output=true` but `disable-copy=true` (the copy/exfil gate).

## Requirements

- A `guacd` built with the SSH/telnet/kubernetes protocols and text-output,
  listening on `127.0.0.1:4822` (override with `GUACD_HOST` / `GUACD_PORT`).
- `python3`, `socat`, and the python3 `websockets` package.
- An `sshd` reachable for the SSH case, accepting the configured credentials.

## Running

```sh
# Defaults: SSH_HOST=127.0.0.1 SSH_PORT=22 SSH_USER=tester SSH_PASS=testpass123
./text-output-e2e.sh

# Point the SSH case at a different target:
SSH_HOST=10.0.0.5 SSH_USER=alice SSH_PASS=secret ./text-output-e2e.sh
```

The script exits non-zero if any check fails. Individual cases can also be run
by hand, e.g.:

```sh
python3 text-output-guacd-e2e.py ssh \
  '{"hostname":"127.0.0.1","port":"22","username":"tester","password":"testpass123",
    "text-output":"true","command":"printf hi"}' --expect hi
```
