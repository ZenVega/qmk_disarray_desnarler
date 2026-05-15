# SPDX-License-Identifier: MIT
"""
Disarray Desnarler — companion app
-----------------------------------
Listens for F13-F16 keypresses from the macropad and executes
user-configured actions (shell commands, app launches, URLs).

Requires:
  - QMK keymap with KC_F13, KC_F14, KC_F15, KC_F16 on the four keys
  - macOS Accessibility permission for the terminal / app running this script
    System Settings → Privacy & Security → Accessibility
"""

import sys
from config.manager import ConfigManager
from device.listener import DeviceListener
from actions.executor import ActionExecutor
from ui.app import DesnarlerApp


def main():
    config = ConfigManager()
    executor = ActionExecutor()
    app = DesnarlerApp(config, executor)

    def on_key(key_name: str):
        action = config.get_action(key_name)
        # Update UI from the listener thread safely
        app.after(0, lambda: executor.run(action))

    listener = DeviceListener(on_key=on_key)
    listener.start_async()

    app.mainloop()


if __name__ == "__main__":
    main()
