# Remote Serial Gateway

This folder contains a lightweight TCP gateway to access serial ports from a remote machine.

## Why this exists

- Allow your GUI tool to fetch serial logs over SSH.
- Avoid direct shell-based serial access from multiple users.
- Provide deterministic behavior:
  - If port is free: open and stream data.
  - If port is busy: return `busy`.

## Files

- `remote_serial_gateway.py`: gateway server.
- `requirements.txt`: Python dependencies.

## Install on remote machine

```bash
cd src_remote
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Run gateway (remote machine)

```bash
cd src_remote
source .venv/bin/activate
python3 remote_serial_gateway.py --host 127.0.0.1 --port 9026
```

The server binds to `127.0.0.1` by default for safety.

## Connect from local machine via SSH tunnel

```bash
ssh -N -L 19026:127.0.0.1:9026 user@REMOTE_IP
```

Then your GUI/client connects to `127.0.0.1:19026`.

## JSON line protocol

Every message is one JSON object per line.

### Requests

- `ping`
- `status`
- `list_ports`
- `open`
- `close`
- `write`

Example open request:

```json
{"id":1,"action":"open","port":"/dev/ttyUSB0","baudrate":115200,"user":"alice"}
```

Busy response example:

```json
{"id":1,"ok":false,"error":{"code":"busy","message":"Port is busy (...)"},"owner_user":"bob"}
```

Serial data event example:

```json
{"event":"serial_data","port":"/dev/ttyUSB0","data_b64":"SGVsbG8K","timestamp":"2026-03-28T12:00:00+00:00"}
```

## Basic behavior

- Only one owner can open/write/close at a time.
- If another process already holds the serial port, `open` returns `busy`.
- The owner user name is tracked from the `user` request field.
- Busy responses include `owner_user` so GUI can show who currently owns the port.
- When owner disconnects, the gateway closes the serial port.
- Data is streamed as Base64 in `serial_data` events.

## Notes

- On Linux, serial permissions usually require user membership in groups such as `dialout`.
- Recommended deployment is a `systemd` service for auto-restart.
