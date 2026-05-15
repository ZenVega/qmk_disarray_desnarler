# SPDX-License-Identifier: MIT
import threading
from typing import Callable
from pynput import keyboard

# Maps pynput key objects to our internal key names.
# These correspond to F13-F16 in your QMK keymap.
WATCHED_KEYS: dict[keyboard.Key, str] = {
    keyboard.Key.f13: "F13",
    keyboard.Key.f14: "F14",
    keyboard.Key.f15: "F15",
    keyboard.Key.f16: "F16",
}


class DeviceListener:
    """
    Listens globally for F13-F16 keypresses and calls on_key with the key name.
    Runs on a background thread — call start() or run start_async().

    Note: macOS requires Accessibility permissions for pynput to work.
    System Preferences → Privacy & Security → Accessibility → add your terminal / app.
    """

    def __init__(self, on_key: Callable[[str], None]):
        self.on_key = on_key
        self._listener: keyboard.Listener | None = None

    def _on_press(self, key):
        if key in WATCHED_KEYS:
            self.on_key(WATCHED_KEYS[key])

    def start(self):
        """Blocking. Call from a dedicated thread."""
        with keyboard.Listener(on_press=self._on_press) as listener:
            self._listener = listener
            listener.join()

    def start_async(self) -> threading.Thread:
        """Starts listening in a daemon thread and returns it."""
        t = threading.Thread(target=self.start, daemon=True, name="desnarler-listener")
        t.start()
        return t

    def stop(self):
        if self._listener:
            self._listener.stop()
