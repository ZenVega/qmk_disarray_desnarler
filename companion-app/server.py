# SPDX-License-Identifier: MIT
"""
Disarray Desnarler — web companion server
------------------------------------------
FastAPI backend that:
  - Serves the web UI from /static
  - Exposes REST endpoints to read/write key config
  - Pushes keypress events to the browser via WebSocket
  - Runs the pynput listener on a daemon thread

Run:
  uvicorn server:app --host 127.0.0.1 --port 8765 --reload
  then open http://localhost:8765
"""

import asyncio
from contextlib import asynccontextmanager
from typing import Optional, Set

import uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from config.manager import ConfigManager
from device.listener import DeviceListener
from actions.executor import ActionExecutor


# ─── State ───────────────────────────────────────────────────────────────────

config = ConfigManager()
executor = ActionExecutor()
clients: Set[WebSocket] = set()
main_loop: Optional[asyncio.AbstractEventLoop] = None


# ─── WebSocket broadcast ─────────────────────────────────────────────────────

async def broadcast(data: dict):
    dead = set()
    for ws in list(clients):
        try:
            await ws.send_json(data)
        except Exception:
            dead.add(ws)
    clients.difference_update(dead)


def on_key(key_name: str):
    """Called from the pynput thread when a mapped key is pressed."""
    executor.run(config.get_action(key_name))
    if main_loop:
        asyncio.run_coroutine_threadsafe(
            broadcast({"type": "keypress", "key": key_name}),
            main_loop,
        )


# ─── Lifespan: start listener before serving ─────────────────────────────────

@asynccontextmanager
async def lifespan(app: FastAPI):
    global main_loop
    main_loop = asyncio.get_event_loop()
    DeviceListener(on_key=on_key).start_async()
    yield


# ─── App ─────────────────────────────────────────────────────────────────────

app = FastAPI(lifespan=lifespan)
app.mount("/static", StaticFiles(directory="static"), name="static")


@app.get("/")
async def root():
    return FileResponse("static/index.html")


# ─── REST API ────────────────────────────────────────────────────────────────

@app.get("/api/config")
async def get_config():
    return config.get_all_keys()


class KeyUpdate(BaseModel):
    label: str
    action: dict


@app.put("/api/keys/{key_name}")
async def update_key(key_name: str, body: KeyUpdate):
    config.set_key(key_name, body.label, body.action)
    config.save()
    # Notify all open UIs so they refresh without a page reload
    await broadcast({"type": "config_updated", "key": key_name})
    return {"ok": True}


# ─── WebSocket ───────────────────────────────────────────────────────────────

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    try:
        while True:
            await websocket.receive_text()  # keep-alive ping from client
    except WebSocketDisconnect:
        clients.discard(websocket)


# ─── Entry point ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    uvicorn.run("server:app", host="127.0.0.1", port=8765, reload=False)
