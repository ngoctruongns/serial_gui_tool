#!/usr/bin/env python3
"""Remote serial gateway.

This server is designed to run on the remote machine.
Clients should connect through SSH local port forwarding.

Protocol: JSON lines (one JSON object per line, UTF-8).
Request shape:
  {"id": 1, "action": "list_ports", ...}
Response shape:
  {"id": 1, "ok": true, ...}
Async event shape:
  {"event": "serial_data", "port": "/dev/ttyUSB0", "data_b64": "...", "timestamp": "..."}
"""

from __future__ import annotations

import argparse
import asyncio
import base64
import datetime as dt
import json
import signal
from dataclasses import dataclass
from typing import Any, Dict, Optional, Set, Tuple

import serial
from serial import SerialException
from serial.tools import list_ports


def utc_now_iso() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def make_error(req_id: Any, code: str, message: str) -> Dict[str, Any]:
    return {"id": req_id, "ok": False, "error": {"code": code, "message": message}}


def make_ok(req_id: Any, **fields: Any) -> Dict[str, Any]:
    payload = {"id": req_id, "ok": True}
    payload.update(fields)
    return payload


@dataclass(frozen=True)
class ClientRef:
    peer: str


class RemoteSerialGateway:
    def __init__(self, host: str, port: int, read_chunk_size: int = 4096) -> None:
        self.host = host
        self.port = port
        self.read_chunk_size = read_chunk_size

        self._server: Optional[asyncio.AbstractServer] = None
        self._clients: Set[Tuple[asyncio.StreamReader, asyncio.StreamWriter, ClientRef]] = set()

        self._serial: Optional[serial.Serial] = None
        self._active_port: Optional[str] = None
        self._owner: Optional[ClientRef] = None
        self._owner_user: Optional[str] = None
        self._serial_reader_task: Optional[asyncio.Task[None]] = None
        self._serial_lock = asyncio.Lock()

    async def start(self) -> None:
        self._server = await asyncio.start_server(self._on_client_connected, self.host, self.port)
        sockets = self._server.sockets or []
        bound = ", ".join(str(sock.getsockname()) for sock in sockets)
        print(f"[gateway] listening on {bound}")

    async def stop(self) -> None:
        if self._serial_reader_task:
            self._serial_reader_task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await self._serial_reader_task
            self._serial_reader_task = None

        await self._close_serial(force=True)

        for _, writer, _ in list(self._clients):
            writer.close()
            with contextlib.suppress(Exception):
                await writer.wait_closed()
        self._clients.clear()

        if self._server is not None:
            self._server.close()
            await self._server.wait_closed()
            self._server = None

    async def run_forever(self) -> None:
        await self.start()
        assert self._server is not None
        async with self._server:
            await self._server.serve_forever()

    async def _on_client_connected(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peername = writer.get_extra_info("peername")
        peer = f"{peername[0]}:{peername[1]}" if peername else "unknown"
        client = ClientRef(peer=peer)
        self._clients.add((reader, writer, client))
        print(f"[gateway] client connected: {client.peer}")

        await self._write_json(writer, {"event": "hello", "server_time": utc_now_iso()})

        try:
            while True:
                line = await reader.readline()
                if not line:
                    break

                if len(line) > 128 * 1024:
                    await self._write_json(writer, make_error(None, "invalid_request", "Request line too large"))
                    continue

                try:
                    request = json.loads(line.decode("utf-8"))
                except Exception:
                    await self._write_json(writer, make_error(None, "invalid_json", "Invalid JSON payload"))
                    continue

                response = await self._handle_request(client, request)
                await self._write_json(writer, response)
        finally:
            print(f"[gateway] client disconnected: {client.peer}")
            await self._cleanup_disconnected_client(client)
            self._clients = {item for item in self._clients if item[2] != client}
            writer.close()
            with contextlib.suppress(Exception):
                await writer.wait_closed()

    async def _handle_request(self, client: ClientRef, request: Dict[str, Any]) -> Dict[str, Any]:
        req_id = request.get("id")
        action = request.get("action")

        if not isinstance(action, str) or not action:
            return make_error(req_id, "invalid_request", "Missing or invalid 'action'")

        if action == "ping":
            return make_ok(req_id, server_time=utc_now_iso())

        if action == "status":
            return make_ok(
                req_id,
                active_port=self._active_port,
                is_open=bool(self._serial and self._serial.is_open),
                owner=(self._owner.peer if self._owner else None),
                owner_user=self._owner_user,
            )

        if action == "list_ports":
            ports = []
            for p in list_ports.comports():
                ports.append(
                    {
                        "device": p.device,
                        "description": p.description,
                        "hwid": p.hwid,
                        "manufacturer": p.manufacturer,
                        "serial_number": p.serial_number,
                    }
                )
            return make_ok(req_id, ports=ports)

        if action == "open":
            return await self._handle_open(client, req_id, request)

        if action == "close":
            return await self._handle_close(client, req_id)

        if action == "write":
            return await self._handle_write(client, req_id, request)

        return make_error(req_id, "unknown_action", f"Unsupported action: {action}")

    async def _handle_open(self, client: ClientRef, req_id: Any, request: Dict[str, Any]) -> Dict[str, Any]:
        port = request.get("port")
        if not isinstance(port, str) or not port:
            return make_error(req_id, "invalid_request", "Missing 'port'")

        baudrate = int(request.get("baudrate", 115200))
        bytesize = int(request.get("bytesize", 8))
        parity = str(request.get("parity", "N")).upper()
        stopbits = float(request.get("stopbits", 1))
        timeout_ms = int(request.get("timeout_ms", 100))
        owner_user = str(request.get("user", "")).strip() or client.peer

        async with self._serial_lock:
            if self._serial and self._serial.is_open:
                if self._active_port == port and self._owner == client:
                    return make_ok(
                        req_id,
                        message="Port already open by this client",
                        port=port,
                        owner_user=self._owner_user,
                    )
                busy_resp = make_error(
                    req_id,
                    "busy",
                    f"Port is busy (owner={self._owner.peer if self._owner else 'unknown'}, port={self._active_port})",
                )
                busy_resp["owner_user"] = self._owner_user
                return busy_resp

            try:
                ser = serial.Serial(
                    port=port,
                    baudrate=baudrate,
                    bytesize=bytesize,
                    parity=parity,
                    stopbits=stopbits,
                    timeout=timeout_ms / 1000.0,
                    exclusive=True,
                )
            except SerialException as exc:
                msg = str(exc)
                lower = msg.lower()
                if "busy" in lower or "resource temporarily unavailable" in lower or "permission denied" in lower:
                    return make_error(req_id, "busy", f"Cannot open port {port}: {msg}")
                return make_error(req_id, "open_failed", f"Cannot open port {port}: {msg}")
            except Exception as exc:
                return make_error(req_id, "open_failed", f"Cannot open port {port}: {exc}")

            self._serial = ser
            self._active_port = port
            self._owner = client
            self._owner_user = owner_user
            self._serial_reader_task = asyncio.create_task(self._serial_reader_loop())

        return make_ok(req_id, message="Port opened", port=port, owner_user=self._owner_user)

    async def _handle_close(self, client: ClientRef, req_id: Any) -> Dict[str, Any]:
        async with self._serial_lock:
            if not self._serial or not self._serial.is_open:
                return make_ok(req_id, message="No active port")
            if self._owner != client:
                return make_error(req_id, "forbidden", "Only owner can close the port")

        await self._close_serial(force=False)
        return make_ok(req_id, message="Port closed")

    async def _handle_write(self, client: ClientRef, req_id: Any, request: Dict[str, Any]) -> Dict[str, Any]:
        async with self._serial_lock:
            if not self._serial or not self._serial.is_open:
                return make_error(req_id, "not_open", "No active serial port")
            if self._owner != client:
                return make_error(req_id, "forbidden", "Only owner can write")

            encoding = str(request.get("encoding", "utf8")).lower()
            payload = request.get("data", "")

            try:
                if encoding == "utf8":
                    if not isinstance(payload, str):
                        return make_error(req_id, "invalid_request", "For utf8, 'data' must be a string")
                    data_bytes = payload.encode("utf-8")
                elif encoding == "hex":
                    if not isinstance(payload, str):
                        return make_error(req_id, "invalid_request", "For hex, 'data' must be a string")
                    cleaned = payload.replace(" ", "")
                    data_bytes = bytes.fromhex(cleaned)
                elif encoding == "base64":
                    if not isinstance(payload, str):
                        return make_error(req_id, "invalid_request", "For base64, 'data' must be a string")
                    data_bytes = base64.b64decode(payload)
                else:
                    return make_error(req_id, "invalid_request", "Unsupported encoding")
            except Exception as exc:
                return make_error(req_id, "invalid_payload", f"Failed to decode payload: {exc}")

            try:
                written = self._serial.write(data_bytes)
            except Exception as exc:
                return make_error(req_id, "write_failed", f"Write failed: {exc}")

        return make_ok(req_id, bytes_written=written)

    async def _serial_reader_loop(self) -> None:
        try:
            while True:
                ser = self._serial
                if ser is None or not ser.is_open:
                    break

                data = await asyncio.to_thread(ser.read, self.read_chunk_size)
                if not data:
                    await asyncio.sleep(0.01)
                    continue

                event = {
                    "event": "serial_data",
                    "port": self._active_port,
                    "data_b64": base64.b64encode(data).decode("ascii"),
                    "timestamp": utc_now_iso(),
                }
                await self._broadcast_json(event)
        except asyncio.CancelledError:
            pass
        except Exception as exc:
            await self._broadcast_json(
                {
                    "event": "serial_error",
                    "port": self._active_port,
                    "timestamp": utc_now_iso(),
                    "message": str(exc),
                }
            )
        finally:
            await self._close_serial(force=True)

    async def _cleanup_disconnected_client(self, client: ClientRef) -> None:
        async with self._serial_lock:
            if self._owner == client:
                await self._close_serial(force=False)

    async def _close_serial(self, force: bool) -> None:
        task = self._serial_reader_task
        if task and not task.done() and not force:
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task

        self._serial_reader_task = None

        if self._serial is not None:
            try:
                if self._serial.is_open:
                    self._serial.close()
            except Exception:
                pass

        self._serial = None
        self._active_port = None
        self._owner = None
        self._owner_user = None

    async def _write_json(self, writer: asyncio.StreamWriter, payload: Dict[str, Any]) -> None:
        data = (json.dumps(payload, ensure_ascii=True, separators=(",", ":")) + "\n").encode("utf-8")
        writer.write(data)
        await writer.drain()

    async def _broadcast_json(self, payload: Dict[str, Any]) -> None:
        stale = []
        for _, writer, _ in self._clients:
            try:
                await self._write_json(writer, payload)
            except Exception:
                stale.append(writer)

        if stale:
            self._clients = {item for item in self._clients if item[1] not in stale}


# contextlib is imported late to keep top-level imports compact.
import contextlib  # noqa: E402


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Remote Serial Gateway")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=9026, help="Bind TCP port (default: 9026)")
    parser.add_argument(
        "--read-chunk",
        type=int,
        default=4096,
        help="Max bytes per serial read iteration (default: 4096)",
    )
    return parser


async def main_async(args: argparse.Namespace) -> None:
    gateway = RemoteSerialGateway(host=args.host, port=args.port, read_chunk_size=args.read_chunk)

    loop = asyncio.get_running_loop()
    stop_event = asyncio.Event()

    def _stop() -> None:
        stop_event.set()

    for sig in (signal.SIGINT, signal.SIGTERM):
        with contextlib.suppress(NotImplementedError):
            loop.add_signal_handler(sig, _stop)

    await gateway.start()

    await stop_event.wait()
    print("[gateway] stopping...")
    await gateway.stop()


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()
    try:
        asyncio.run(main_async(args))
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
