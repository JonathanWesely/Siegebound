#!/usr/bin/env python3
"""
Blender MCP stdio bridge (dual-framing) — connects an MCP client to Blender's
official Lab MCP add-on TCP server at localhost:9876.

Why this exists (do not "simplify" back to the add-on's own mcp_bridge.py):
The add-on ships `.../extensions/user_default/mcp/mcp_bridge.py`, but it speaks
ONLY Content-Length (LSP-style) framing over stdio. Claude Desktop uses that
framing, so it works there — but Claude Code (and the MCP stdio spec in general)
use NEWLINE-DELIMITED JSON-RPC, which the add-on bridge never answers, so its
`initialize` hangs and Claude Code reports a 30s connection timeout.

This bridge AUTO-DETECTS the client's framing on the first message and replies
in the same framing, so it works with both Claude Code (newline) and Claude
Desktop (Content-Length). The Blender-side protocol (null-byte-delimited JSON
execute requests to :9876) and the tool definitions are copied verbatim from the
add-on's mcp_bridge.py.

Configured in .mcp.json:
  "blender": {
    "command": ".../Blender 5.1/5.1/python/bin/python.exe",
    "args": ["-u", "-X", "utf8", ".../GitClaudeUnrealTest/Tools/blender_mcp_bridge.py"],
    "env": { "PYTHONUTF8": "1", "PYTHONIOENCODING": "utf-8" }
  }
Any Python 3.10+ works (stdlib only — no bpy import); Blender's bundled Python is
used just to match the add-on's recommended setup.
"""

import io
import json
import socket
import sys

# Force UTF-8 on Windows so stderr never emits non-UTF-8 bytes that crash the
# client's reader thread.
if sys.platform == "win32":
    sys.stderr = io.TextIOWrapper(
        sys.stderr.buffer, encoding="utf-8", errors="replace", line_buffering=True
    )

BLENDER_HOST = "localhost"
BLENDER_PORT = 9876
_MAX_RESPONSE_BYTES = 10 * 1024 * 1024  # 10 MiB

# Binary stdin/stdout for correct byte-accurate framing on Windows.
_stdin = sys.stdin.buffer
_stdout = sys.stdout.buffer

# How the connected client frames messages: "line" (newline-delimited JSON, the
# MCP stdio default — Claude Code) or "header" (Content-Length — Claude Desktop).
# Detected from the first message; default to the spec default.
_client_framing = "line"


def _log(msg: str) -> None:
    sys.stderr.write(f"[blender-mcp-bridge] {msg}\n")
    sys.stderr.flush()


# ---------------------------------------------------------------------------
# MCP transport — auto-detect newline-delimited vs Content-Length framing

def _read_exact(n: int) -> bytes:
    """Read exactly n bytes from stdin (loops in case the pipe under-reads)."""
    buf = bytearray()
    while len(buf) < n:
        chunk = _stdin.read(n - len(buf))
        if not chunk:
            break  # EOF
        buf.extend(chunk)
    return bytes(buf)


def _read_message() -> dict | None:
    """Read one JSON-RPC message from stdin, detecting the client's framing."""
    global _client_framing

    # Read the first non-blank line (blank lines can separate newline-framed msgs).
    while True:
        raw = _stdin.readline()
        if not raw:
            return None  # EOF
        line = raw.decode("utf-8").rstrip("\r\n")
        if line != "":
            break

    if line.lower().startswith("content-length:"):
        # Content-Length (LSP) framing — parse headers, then read the body.
        _client_framing = "header"
        headers: dict[str, str] = {}
        key, _, value = line.partition(":")
        headers[key.strip().lower()] = value.strip()
        while True:
            raw = _stdin.readline()
            if not raw:
                return None
            hline = raw.decode("utf-8").rstrip("\r\n")
            if hline == "":
                break  # blank line ends headers
            if ":" in hline:
                k, _, v = hline.partition(":")
                headers[k.strip().lower()] = v.strip()
        length = int(headers.get("content-length", 0))
        if length <= 0:
            return None
        return json.loads(_read_exact(length).decode("utf-8"))

    # Newline-delimited JSON — the line itself is the whole message.
    _client_framing = "line"
    return json.loads(line)


def _write_message(msg: dict) -> None:
    """Write one JSON-RPC message to stdout in the client's framing."""
    body = json.dumps(msg, ensure_ascii=False).encode("utf-8")
    if _client_framing == "header":
        _stdout.write(f"Content-Length: {len(body)}\r\n\r\n".encode("utf-8") + body)
    else:
        # MCP newline-delimited: one JSON object per line, no embedded newlines.
        _stdout.write(body + b"\n")
    _stdout.flush()


# ---------------------------------------------------------------------------
# Blender TCP client (verbatim from the add-on's mcp_bridge.py)

def _blender_execute(code: str, strict_json: bool = False) -> dict:
    """Send Python code to Blender's TCP server and return the response dict."""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(30.0)
        sock.connect((BLENDER_HOST, BLENDER_PORT))

        request = json.dumps({
            "type": "execute",
            "code": code,
            "strict_json": strict_json,
        })
        sock.sendall(request.encode("utf-8") + b"\0")

        buf = bytearray()
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buf.extend(chunk)
            if b"\0" in buf or len(buf) > _MAX_RESPONSE_BYTES:
                break

        sock.close()
        return json.loads(bytes(buf.split(b"\0")[0]).decode("utf-8"))

    except ConnectionRefusedError:
        return {
            "status": "error",
            "message": (
                "Cannot connect to Blender at localhost:9876. "
                "Ensure Blender is open and the MCP add-on is enabled "
                "(Edit > Preferences > Get Extensions > MCP) with Online Access on."
            ),
        }
    except socket.timeout:
        return {"status": "error", "message": "Timed out waiting for Blender."}
    except Exception as exc:
        return {"status": "error", "message": f"Bridge error: {exc}"}


# ---------------------------------------------------------------------------
# Tool definitions (verbatim from the add-on's mcp_bridge.py)

_TOOLS = [
    {
        "name": "execute_blender_code",
        "description": (
            "Execute Python code in the running Blender instance. "
            "Full access to bpy (Blender Python API) is available. "
            "Store any return data in a variable named `result` as a "
            "JSON-serialisable dict.\n"
            "Example:\n"
            "  import bpy\n"
            "  result = {\"objects\": [o.name for o in bpy.data.objects]}"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "code": {
                    "type": "string",
                    "description": "Python code to run inside Blender.",
                }
            },
            "required": ["code"],
        },
    },
    {
        "name": "get_scene_info",
        "description": (
            "Return a JSON summary of the current Blender scene: "
            "objects, render settings, active object, and frame range."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    {
        "name": "get_object_info",
        "description": "Return detailed properties of a named Blender object.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {
                    "type": "string",
                    "description": "Object name as shown in the Blender outliner.",
                }
            },
            "required": ["name"],
        },
    },
]

_SCENE_INFO_CODE = """
import bpy
scene = bpy.context.scene
active = bpy.context.active_object
result = {
    "scene_name": scene.name,
    "frame": scene.frame_current,
    "frame_range": [scene.frame_start, scene.frame_end],
    "render_engine": scene.render.engine,
    "resolution": [scene.render.resolution_x, scene.render.resolution_y],
    "active_object": active.name if active else None,
    "selected_objects": [o.name for o in bpy.context.selected_objects],
    "objects": [
        {
            "name": o.name,
            "type": o.type,
            "location": list(o.location),
            "visible": o.visible_get(),
        }
        for o in scene.objects
    ],
}
"""

_OBJECT_INFO_CODE = """
import bpy
name = {name!r}
obj = bpy.data.objects.get(name)
if obj is None:
    result = {{"error": f"Object {{name!r}} not found"}}
else:
    result = {{
        "name": obj.name,
        "type": obj.type,
        "location": list(obj.location),
        "rotation_euler": list(obj.rotation_euler),
        "scale": list(obj.scale),
        "visible": obj.visible_get(),
        "data_name": obj.data.name if obj.data else None,
        "material_slots": [
            ms.material.name if ms.material else None
            for ms in obj.material_slots
        ],
    }}
"""


# ---------------------------------------------------------------------------
# Request handler

def _handle(request: dict) -> dict | None:
    method = request.get("method", "")
    req_id = request.get("id")
    params = request.get("params") or {}

    if method == "initialize":
        # Echo the client's requested protocol version for maximum compatibility.
        client_version = params.get("protocolVersion", "2024-11-05")
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": client_version,
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "blender-mcp-bridge", "version": "1.1.0"},
            },
        }

    # Any notification (no id) needs no response: `initialized`,
    # `notifications/initialized`, `notifications/cancelled`, etc.
    if method == "initialized" or method.startswith("notifications/"):
        return None

    if method in ("ping", "$/cancelRequest"):
        return {"jsonrpc": "2.0", "id": req_id, "result": {}} if req_id is not None else None

    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": req_id, "result": {"tools": _TOOLS}}

    if method == "tools/call":
        tool = params.get("name", "")
        args = params.get("arguments") or {}

        if tool == "get_scene_info":
            resp = _blender_execute(_SCENE_INFO_CODE, strict_json=True)
        elif tool == "get_object_info":
            code = _OBJECT_INFO_CODE.format(name=args.get("name", ""))
            resp = _blender_execute(code, strict_json=True)
        elif tool == "execute_blender_code":
            resp = _blender_execute(args.get("code", ""), strict_json=False)
        else:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {"code": -32601, "message": f"Unknown tool: {tool}"},
            }

        is_error = resp.get("status") == "error"
        parts: list[str] = []
        if resp.get("stdout"):
            parts.append(resp["stdout"])
        if resp.get("stderr"):
            parts.append(resp["stderr"])
        if is_error:
            parts.append(f"Error: {resp.get('message', 'Unknown error')}")
        else:
            result_data = resp.get("result", resp)
            parts.append(json.dumps(result_data, indent=2, ensure_ascii=False))

        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "content": [{"type": "text", "text": "\n".join(parts)}],
                "isError": is_error,
            },
        }

    # Unknown method with an id → error; unknown notification → ignore.
    if req_id is not None:
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "error": {"code": -32601, "message": f"Method not found: {method}"},
        }
    return None


# ---------------------------------------------------------------------------
# Main loop

def main() -> None:
    _log("Blender MCP bridge starting (dual-framing). Waiting for client...")
    while True:
        try:
            request = _read_message()
        except Exception as exc:
            _log(f"Read error: {exc}")
            break
        if request is None:
            _log("stdin closed, exiting.")
            break
        _log(f"<- {request.get('method', '?')} id={request.get('id')} [{_client_framing}]")
        try:
            response = _handle(request)
        except Exception as exc:
            response = {
                "jsonrpc": "2.0",
                "id": request.get("id"),
                "error": {"code": -32603, "message": f"Internal error: {exc}"},
            }
        if response is not None:
            _log(f"-> id={response.get('id')}")
            try:
                _write_message(response)
            except Exception as exc:
                _log(f"Write error: {exc}")
                break


if __name__ == "__main__":
    main()
