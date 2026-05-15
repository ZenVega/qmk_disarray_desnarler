# SPDX-License-Identifier: MIT
import subprocess
import webbrowser
import platform
from typing import Optional


class ActionExecutor:
    """
    Executes an action dict. Supported types:

      shell  → runs a shell command via /bin/sh
      app    → launches a macOS app by name (open -a) or Linux binary
      url    → opens a URL in the default browser
    """

    def run(self, action: Optional[dict]):
        if not action:
            return
        dispatch = {
            "shell": self._run_shell,
            "app":   self._launch_app,
            "url":   self._open_url,
        }
        handler = dispatch.get(action.get("type", ""))
        if handler:
            handler(action)

    def _run_shell(self, action: dict):
        command = action.get("command", "").strip()
        if command:
            subprocess.Popen(command, shell=True, executable="/bin/zsh")

    def _launch_app(self, action: dict):
        app = action.get("app", "").strip()
        if not app:
            return
        if platform.system() == "Darwin":
            subprocess.Popen(["open", "-a", app])
        else:
            # Linux: try running the binary directly
            subprocess.Popen([app])

    def _open_url(self, action: dict):
        url = action.get("url", "").strip()
        if url:
            webbrowser.open(url)
