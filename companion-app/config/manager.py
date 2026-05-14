# SPDX-License-Identifier: MIT
import json
from pathlib import Path
from typing import Optional

KEY_NAMES = ["F13", "F14", "F15", "F16"]

DEFAULT_CONFIG = {
    "version": 1,
    "keys": {
        "F13": {"label": "Key 1", "action": {"type": "shell", "command": ""}},
        "F14": {"label": "Key 2", "action": {"type": "shell", "command": ""}},
        "F15": {"label": "Key 3", "action": {"type": "shell", "command": ""}},
        "F16": {"label": "Key 4", "action": {"type": "shell", "command": ""}},
    },
}

CONFIG_PATH = Path.home() / ".desnarler" / "config.json"


class ConfigManager:
    def __init__(self, path: Path = CONFIG_PATH):
        self.path = path
        self.config = self._load()

    def _load(self) -> dict:
        if self.path.exists():
            try:
                with open(self.path) as f:
                    return json.load(f)
            except (json.JSONDecodeError, OSError):
                pass
        return json.loads(json.dumps(DEFAULT_CONFIG))  # deep copy

    def save(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with open(self.path, "w") as f:
            json.dump(self.config, f, indent=2)

    def get_action(self, key_name: str) -> Optional[dict]:
        return self.config["keys"].get(key_name, {}).get("action")

    def get_key(self, key_name: str) -> dict:
        return self.config["keys"].get(key_name, {})

    def set_key(self, key_name: str, label: str, action: dict):
        self.config["keys"][key_name] = {"label": label, "action": action}

    def get_all_keys(self) -> dict:
        return self.config["keys"]
