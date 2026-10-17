#!/usr/bin/env python3
"""
Manual smoke test for Guacamole terminal text-output mode.

This test logs into a Guacamole web application, opens a connection through the
WebSocket tunnel, and verifies that the server opens the expected outbound
STDOUT pipe. By default, the test then sends a harmless printf command through
the Guacamole keyboard protocol and verifies that the command output is
received through that STDOUT pipe. For non-interactive or intentionally
unreachable validation targets, --pipe-only can be used to stop after the pipe
is observed.

The script requires the third-party "websocket-client" Python package.
"""

import argparse
import base64
import json
import os
import sys
import time
import urllib.parse
import urllib.request

try:
    import websocket
except ImportError as exc:  # pragma: no cover - manual dependency check
    raise SystemExit(
        'Missing dependency: install the "websocket-client" Python package.'
    ) from exc


def guac_instruction(opcode, *args):
    """Return a Guacamole protocol instruction."""

    elements = (opcode,) + tuple(str(arg) for arg in args)
    return ','.join(f'{len(element)}.{element}' for element in elements) + ';'


def parse_guac_instructions(data):
    """Yield parsed Guacamole protocol instructions from the given message."""

    index = 0
    length = len(data)

    while index < length:
        instruction = []

        while True:
            dot = data.find('.', index)
            if dot < 0:
                raise ValueError('Incomplete Guacamole instruction element.')

            element_length = int(data[index:dot])
            start = dot + 1
            end = start + element_length
            instruction.append(data[start:end])

            separator = data[end]
            index = end + 1

            if separator == ';':
                yield instruction
                break

            if separator != ',':
                raise ValueError(f'Unexpected Guacamole separator: {separator!r}')


def rest_json(url, data=None):
    """Submit an HTTP request and parse the JSON response."""

    with urllib.request.urlopen(url, data=data, timeout=10) as response:
        return json.load(response)


def login(base_url, username, password):
    """Authenticate via REST and return the auth token."""

    data = urllib.parse.urlencode({
        'username': username,
        'password': password,
    }).encode('utf-8')

    return rest_json(f'{base_url}/api/tokens', data=data)['authToken']


def send_text_as_keys(ws, text, delay):
    """Send text through Guacamole's keyboard protocol."""

    for char in text:
        keysym = 0xFF0D if char == '\n' else ord(char)
        ws.send(guac_instruction('key', keysym, 1))
        ws.send(guac_instruction('key', keysym, 0))
        if delay:
            time.sleep(delay)


def run_smoke_test(args):
    base_url = args.url.rstrip('/')

    if base_url.startswith('https://'):
        ws_base_url = 'wss://' + base_url[len('https://'):]
    elif base_url.startswith('http://'):
        ws_base_url = 'ws://' + base_url[len('http://'):]
    else:
        raise SystemExit('URL must begin with http:// or https://')

    token = login(base_url, args.username, args.password)

    tunnel_params = urllib.parse.urlencode({
        'token': token,
        'GUAC_DATA_SOURCE': args.data_source,
        'GUAC_ID': args.connection_id,
        'GUAC_TYPE': 'c',
    })

    tunnel_url = f'{ws_base_url}/websocket-tunnel?{tunnel_params}'
    origin = urllib.parse.urlsplit(base_url)._replace(path='', query='', fragment='').geturl()

    print(f'Opening connection {args.connection_id!r} via {base_url} ...')
    ws = websocket.create_connection(tunnel_url, timeout=args.connect_timeout,
                                     origin=origin)
    ws.settimeout(args.receive_timeout)

    streams = {}
    stdout = bytearray()
    command_sent = False
    command = args.command or f"printf '\\n{args.marker}\\n'"
    start = time.time()

    try:
        while time.time() - start < args.timeout:
            try:
                message = ws.recv()
            except TimeoutError:
                continue
            except websocket.WebSocketTimeoutException:
                continue

            for instruction in parse_guac_instructions(message):
                opcode = instruction[0]

                if opcode == 'pipe' and len(instruction) >= 4:
                    stream_index = instruction[1]
                    mimetype = instruction[2]
                    name = instruction[3]
                    streams[stream_index] = name
                    print(f'PIPE stream={stream_index} mimetype={mimetype} name={name}')

                elif opcode == 'blob' and len(instruction) >= 3:
                    stream_index = instruction[1]
                    if streams.get(stream_index) == args.pipe_name:
                        stdout.extend(base64.b64decode(instruction[2]))
                        ws.send(guac_instruction('ack', stream_index, 'OK', 0))

                elif opcode == 'end' and len(instruction) >= 2:
                    stream_index = instruction[1]
                    if streams.get(stream_index) == args.pipe_name:
                        ws.send(guac_instruction('ack', stream_index, 'OK', 0))

                elif opcode == 'sync' and len(instruction) >= 2:
                    ws.send(guac_instruction('sync', instruction[1]))

            if args.pipe_name in streams.values() and args.pipe_only:
                print('RESULT: tunnel STDOUT pipe opened')
                return 0

            if args.pipe_name in streams.values() and not command_sent:
                prompt = args.prompt.encode('utf-8')
                if prompt and prompt not in stdout:
                    continue
                send_text_as_keys(ws, command + '\n', args.key_delay)
                command_sent = True

            marker = args.marker.encode('utf-8')
            if b'\n' + marker in stdout or b'\r\n' + marker in stdout:
                text = stdout.decode('utf-8', errors='replace')
                print(f'STDOUT bytes: {len(stdout)}')
                print('STDOUT tail:')
                print(text[-args.tail_bytes:])
                print('RESULT: tunnel STDOUT pipe smoke test passed')
                return 0

    finally:
        ws.close()

    text = stdout.decode('utf-8', errors='replace')
    print(f'STDOUT bytes: {len(stdout)}')
    print('STDOUT tail:')
    print(text[-args.tail_bytes:])
    print('RESULT: tunnel STDOUT pipe smoke test failed', file=sys.stderr)
    return 1


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description='Smoke-test Guacamole terminal text-output over WebSocket.'
    )
    parser.add_argument('--url', default=os.environ.get('GUAC_URL'),
                        help='Base Guacamole URL. May also be set with GUAC_URL.')
    parser.add_argument('--username', default=os.environ.get('GUAC_USERNAME'),
                        help='Guacamole username. May also be set with GUAC_USERNAME.')
    parser.add_argument('--password', default=os.environ.get('GUAC_PASSWORD'),
                        help='Guacamole password. May also be set with GUAC_PASSWORD.')
    parser.add_argument('--data-source',
                        default=os.environ.get('GUAC_DATA_SOURCE', 'postgresql'),
                        help='Guacamole datasource. Default: %(default)s')
    parser.add_argument('--connection-id',
                        default=os.environ.get('GUAC_CONNECTION_ID'),
                        help='Guacamole connection identifier. May also be set with GUAC_CONNECTION_ID.')
    parser.add_argument('--pipe-name', default='STDOUT',
                        help='Expected outbound pipe stream name. Default: %(default)s')
    parser.add_argument('--marker', default='GUAC_TEXT_OUTPUT_SMOKE_OK',
                        help='Marker expected in command output. Default: %(default)s')
    parser.add_argument('--command', default=None,
                        help='Command to type. It must print --marker to stdout.')
    parser.add_argument('--prompt', default='$ ',
                        help='Prompt bytes to wait for before typing, or empty to type immediately. Default: %(default)r')
    parser.add_argument('--pipe-only', action='store_true',
                        help='Pass if observing the expected pipe is sufficient.')
    parser.add_argument('--timeout', type=float, default=15,
                        help='Overall timeout in seconds. Default: %(default)s')
    parser.add_argument('--connect-timeout', type=float, default=5,
                        help='WebSocket connect timeout. Default: %(default)s')
    parser.add_argument('--receive-timeout', type=float, default=1,
                        help='WebSocket receive timeout. Default: %(default)s')
    parser.add_argument('--key-delay', type=float, default=0.01,
                        help='Delay between key events. Default: %(default)s')
    parser.add_argument('--tail-bytes', type=int, default=1200,
                        help='Bytes of decoded STDOUT tail to print. Default: %(default)s')
    args = parser.parse_args(argv)

    missing = [
        option for option, value in (
            ('--url', args.url),
            ('--username', args.username),
            ('--password', args.password),
            ('--connection-id', args.connection_id),
        )
        if not value
    ]

    if missing:
        parser.error('missing required arguments: ' + ', '.join(missing))

    return args


def main(argv=None):
    return run_smoke_test(parse_args(argv))


if __name__ == '__main__':
    sys.exit(main())
