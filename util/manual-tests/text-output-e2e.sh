#!/bin/bash
#
# End-to-end validation of Guacamole terminal text-output mode against a local
# guacd, for all three terminal protocols (SSH, telnet, kubernetes), covering
# both the positive path (raw ANSI is teed to the STDOUT pipe) and the negative
# path (no pipe when text-output is off or gated by disable-copy).
#
# This is a manual test: it needs a running guacd (127.0.0.1:4822) and creates
# throwaway local servers for telnet (socat) and kubernetes (a mock WebSocket
# exec endpoint). SSH is exercised against a real sshd, which must accept the
# credentials passed below.
#
# Requirements in the environment running this script:
#   - guacd built with the SSH/telnet/kubernetes protocols and text-output
#   - python3, socat, and the python3 "websockets" package
#   - an sshd reachable at SSH_HOST:SSH_PORT accepting SSH_USER/SSH_PASS
#
# Configure the SSH target via environment variables (defaults shown):
#   SSH_HOST=127.0.0.1 SSH_PORT=22 SSH_USER=tester SSH_PASS=testpass123
#
set -u

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DRIVER="$DIR/text-output-guacd-e2e.py"
MOCK="$DIR/text-output-k8s-exec-mock.py"

SSH_HOST="${SSH_HOST:-127.0.0.1}"
SSH_PORT="${SSH_PORT:-22}"
SSH_USER="${SSH_USER:-tester}"
SSH_PASS="${SSH_PASS:-testpass123}"
TELNET_PORT="${TELNET_PORT:-2323}"
K8S_PORT="${K8S_PORT:-8091}"

RESULT=0
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"; pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null; pkill -f "$MOCK" 2>/dev/null' EXIT

# ANSI-emitting program used as the telnet "shell".
ANSI="$TMP/ansi-emit.sh"
cat > "$ANSI" <<'EOS'
#!/bin/bash
printf '\033[35mTELNET-TEXT-OUTPUT marker\033[0m\r\n'
sleep 3
EOS
chmod +x "$ANSI"

hr(){ echo; echo "======================================================"; echo "== $1"; echo "======================================================"; }
check(){ [ "$1" -ne 0 ] && RESULT=1; }

ss -ltn 2>/dev/null | grep -q ':4822' || { echo "guacd is not listening on 127.0.0.1:4822"; exit 3; }

# =========================== POSITIVE PATH ===========================

hr "SSH  (real sshd, command emits red ANSI)  -> expect STDOUT pipe"
python3 "$DRIVER" ssh \
  "{\"hostname\":\"$SSH_HOST\",\"port\":\"$SSH_PORT\",\"username\":\"$SSH_USER\",\"password\":\"$SSH_PASS\",\"text-output\":\"true\",\"command\":\"printf '\\\\033[31mSSH-TEXT-OUTPUT marker\\\\033[0m\\\\r\\\\n'; sleep 2\"}" \
  --secs 10 --expect "SSH-TEXT-OUTPUT marker"
check $?

hr "TELNET  (socat server -> magenta ANSI)  -> expect STDOUT pipe"
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null; sleep 0.3
socat TCP-LISTEN:"$TELNET_PORT",reuseaddr,fork EXEC:"$ANSI",pty,stderr & sleep 0.5
python3 "$DRIVER" telnet \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$TELNET_PORT\",\"text-output\":\"true\"}" \
  --secs 8 --expect "TELNET-TEXT-OUTPUT marker"
check $?
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null

hr "KUBERNETES  (mock exec WS -> green ANSI on channel 1)  -> expect STDOUT pipe"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"true\"}" \
  --secs 8 --expect "K8S-TEXT-OUTPUT"
check $?
pkill -f "$MOCK" 2>/dev/null

# =========================== NEGATIVE PATH ===========================

hr "SSH default-off (no text-output)  -> expect NO pipe"
python3 "$DRIVER" ssh \
  "{\"hostname\":\"$SSH_HOST\",\"port\":\"$SSH_PORT\",\"username\":\"$SSH_USER\",\"password\":\"$SSH_PASS\",\"command\":\"printf hi; sleep 1\"}" \
  --secs 6 --expect-no-pipe
check $?

hr "SSH gate (text-output=true + disable-copy=true)  -> expect NO pipe"
python3 "$DRIVER" ssh \
  "{\"hostname\":\"$SSH_HOST\",\"port\":\"$SSH_PORT\",\"username\":\"$SSH_USER\",\"password\":\"$SSH_PASS\",\"command\":\"printf hi; sleep 1\",\"text-output\":\"true\",\"disable-copy\":\"true\"}" \
  --secs 6 --expect-no-pipe
check $?

hr "TELNET default-off  -> expect NO pipe"
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null; sleep 0.3
socat TCP-LISTEN:"$TELNET_PORT",reuseaddr,fork EXEC:"$ANSI",pty,stderr & sleep 0.5
python3 "$DRIVER" telnet "{\"hostname\":\"127.0.0.1\",\"port\":\"$TELNET_PORT\"}" --secs 5 --expect-no-pipe
check $?
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null

hr "KUBERNETES gate (text-output=true + disable-copy=true)  -> expect NO pipe"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"true\",\"disable-copy\":\"true\"}" \
  --secs 5 --expect-no-pipe
check $?
pkill -f "$MOCK" 2>/dev/null

# =========================== FLOW CONTROL ===========================
# guacd bounds the unacknowledged backlog by BYTES (256 KB), not by blob count,
# so a burst only overflows once its volume exceeds that bound. Each line here
# is padded to 4 KB so that 100 lines (~400 KB) comfortably overruns it while
# the blob count stays well inside its own limit -- this exercises the byte
# bound specifically. With acks, the window keeps draining and everything is
# delivered; without acks, guacd drops the backlog once it overflows.

FLOOD_LINES=100
FLOOD_PAD=4096

hr "FLOW CONTROL: ~400KB flood WITH acks -> all lines delivered (line 99 present)"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" "$FLOOD_LINES" "$FLOOD_PAD" >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"true\"}" \
  --secs 30 --expect "K8S-TEXT-OUTPUT line 99"
check $?
pkill -f "$MOCK" 2>/dev/null

hr "FLOW CONTROL: ~400KB flood WITHOUT acks -> backlog dropped (line 00 present, line 99 absent)"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" "$FLOOD_LINES" "$FLOOD_PAD" >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"true\"}" \
  --secs 30 --no-ack --expect "K8S-TEXT-OUTPUT line 00" --expect-absent "K8S-TEXT-OUTPUT line 99"
check $?
pkill -f "$MOCK" 2>/dev/null

hr "FLOW CONTROL: small unacked burst is NOT dropped (byte bound, not blob count)"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" 60 >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"true\"}" \
  --secs 15 --no-ack --expect "K8S-TEXT-OUTPUT line 59"
check $?
pkill -f "$MOCK" 2>/dev/null

# =========================== RAW MODE ===========================
# text-output=raw is headless: the graphical terminal is not rendered, so the
# raw bytes are still delivered via the STDOUT pipe while the graphical
# instruction stream is suppressed (graphics bytes stay at the tiny init
# residual, well under the --max-graphics bound).

hr "RAW SSH: text delivered, graphics suppressed"
python3 "$DRIVER" ssh \
  "{\"hostname\":\"$SSH_HOST\",\"port\":\"$SSH_PORT\",\"username\":\"$SSH_USER\",\"password\":\"$SSH_PASS\",\"text-output\":\"raw\",\"command\":\"printf '\\\\033[31mRAW-SSH marker\\\\033[0m\\\\r\\\\n'; sleep 2\"}" \
  --secs 10 --expect "RAW-SSH marker" --max-graphics 5000
check $?

hr "RAW TELNET: text delivered, graphics suppressed"
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null; sleep 0.3
socat TCP-LISTEN:"$TELNET_PORT",reuseaddr,fork EXEC:"$ANSI",pty,stderr & sleep 0.5
python3 "$DRIVER" telnet \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$TELNET_PORT\",\"text-output\":\"raw\"}" \
  --secs 8 --expect "TELNET-TEXT-OUTPUT marker" --max-graphics 5000
check $?
pkill -f "TCP-LISTEN:$TELNET_PORT" 2>/dev/null

hr "RAW KUBERNETES: 60-line flood delivered, graphics suppressed"
pkill -f "$MOCK" 2>/dev/null; sleep 0.3
python3 "$MOCK" "$K8S_PORT" 60 >"$TMP/k8s.log" 2>&1 & sleep 1
python3 "$DRIVER" kubernetes \
  "{\"hostname\":\"127.0.0.1\",\"port\":\"$K8S_PORT\",\"use-ssl\":\"false\",\"namespace\":\"default\",\"pod\":\"testpod\",\"exec-command\":\"/bin/sh\",\"text-output\":\"raw\"}" \
  --secs 12 --expect "K8S-TEXT-OUTPUT line 59" --max-graphics 5000
check $?
pkill -f "$MOCK" 2>/dev/null

hr "OVERALL: $([ $RESULT -eq 0 ] && echo ALL-PASS || echo SOME-FAILED)"
exit $RESULT
