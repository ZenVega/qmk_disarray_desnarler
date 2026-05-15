# SPDX-License-Identifier: MIT
import customtkinter as ctk

from config.manager import ConfigManager, KEY_NAMES
from actions.executor import ActionExecutor

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# Physical positions matching the 2×2 layout of the Desnarler
KEY_POSITIONS = ["Top Left", "Top Right", "Bottom Left", "Bottom Right"]

ACTION_TYPES = ["shell", "app", "url"]
ACTION_LABELS = {"shell": "Shell Command", "app": "App Name", "url": "URL"}
ACTION_PLACEHOLDERS = {
    "shell": "e.g. open -a Spotify  or  afplay ~/sound.mp3",
    "app":   "e.g. Spotify",
    "url":   "e.g. https://github.com",
}


def _summarize(action: dict) -> str:
    if not action:
        return "No action"
    t = action.get("type", "")
    val = action.get("command") or action.get("app") or action.get("url") or ""
    if not val:
        return "No action"
    val = val if len(val) <= 32 else val[:30] + "…"
    prefix = {"shell": "$", "app": "▶", "url": "🔗"}.get(t, "")
    return f"{prefix} {val}".strip()


# ─── Edit dialog ─────────────────────────────────────────────────────────────

class EditDialog(ctk.CTkToplevel):
    def __init__(self, parent, key_name: str, config: ConfigManager, on_saved):
        super().__init__(parent)
        self.key_name = key_name
        self.config = config
        self.on_saved = on_saved

        self.title(f"Edit {key_name}")
        self.geometry("420x300")
        self.resizable(False, False)
        self.grab_set()
        self.focus_set()

        key_data = config.get_key(key_name)
        action = key_data.get("action", {})

        pad = {"padx": 24, "pady": (0, 0)}

        ctk.CTkLabel(self, text="Label", font=ctk.CTkFont(size=12)).pack(anchor="w", padx=24, pady=(20, 4))
        self.label_entry = ctk.CTkEntry(self, width=372, placeholder_text="Key label")
        self.label_entry.insert(0, key_data.get("label", key_name))
        self.label_entry.pack(padx=24)

        ctk.CTkLabel(self, text="Action Type", font=ctk.CTkFont(size=12)).pack(anchor="w", padx=24, pady=(16, 4))
        self.type_var = ctk.StringVar(value=action.get("type", "shell"))
        self.type_menu = ctk.CTkOptionMenu(
            self, values=ACTION_TYPES, variable=self.type_var,
            command=self._on_type_change, width=372,
        )
        self.type_menu.pack(padx=24)

        self.value_label = ctk.CTkLabel(self, text="", font=ctk.CTkFont(size=12))
        self.value_label.pack(anchor="w", padx=24, pady=(16, 4))
        self.value_entry = ctk.CTkEntry(self, width=372)
        self.value_entry.pack(padx=24)

        # populate value field from saved action
        val = action.get("command") or action.get("app") or action.get("url") or ""
        self.value_entry.insert(0, val)
        self._on_type_change(self.type_var.get())

        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack(fill="x", padx=24, pady=20)
        ctk.CTkButton(btn_row, text="Cancel", width=100, fg_color="#555", hover_color="#666",
                      command=self.destroy).pack(side="left")
        ctk.CTkButton(btn_row, text="Save", width=100,
                      command=self._save).pack(side="right")

    def _on_type_change(self, value: str):
        self.value_label.configure(text=ACTION_LABELS.get(value, "Value"))
        self.value_entry.configure(placeholder_text=ACTION_PLACEHOLDERS.get(value, ""))

    def _save(self):
        label = self.label_entry.get().strip() or self.key_name
        action_type = self.type_var.get()
        value = self.value_entry.get().strip()

        field = {"shell": "command", "app": "app", "url": "url"}[action_type]
        action = {"type": action_type, field: value}

        self.config.set_key(self.key_name, label, action)
        self.config.save()
        self.on_saved(self.key_name)
        self.destroy()


# ─── Key card ────────────────────────────────────────────────────────────────

class KeyCard(ctk.CTkFrame):
    def __init__(self, parent, key_name: str, position: str,
                 config: ConfigManager, on_edit, on_test, **kwargs):
        super().__init__(parent, corner_radius=12, **kwargs)
        self.key_name = key_name
        self.config = config
        self.on_edit = on_edit
        self.on_test = on_test

        self.configure(width=172, height=140)
        self.pack_propagate(False)

        ctk.CTkLabel(self, text=position,
                     font=ctk.CTkFont(size=10), text_color="gray").pack(pady=(10, 2))

        self.lbl_name = ctk.CTkLabel(self, text="", font=ctk.CTkFont(size=14, weight="bold"))
        self.lbl_name.pack()

        self.lbl_action = ctk.CTkLabel(self, text="", font=ctk.CTkFont(size=10),
                                       text_color="gray", wraplength=150)
        self.lbl_action.pack(pady=(2, 8))

        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack()
        ctk.CTkButton(btn_row, text="Edit", width=70, height=26,
                      command=lambda: on_edit(key_name)).pack(side="left", padx=(0, 4))
        ctk.CTkButton(btn_row, text="▶ Test", width=70, height=26,
                      fg_color="#2d6a2d", hover_color="#3a8a3a",
                      command=lambda: on_test(key_name)).pack(side="left")

        self.refresh()

    def refresh(self):
        key_data = self.config.get_key(self.key_name)
        self.lbl_name.configure(text=key_data.get("label", self.key_name))
        self.lbl_action.configure(text=_summarize(key_data.get("action", {})))


# ─── Main window ─────────────────────────────────────────────────────────────

class DesnarlerApp(ctk.CTk):
    def __init__(self, config: ConfigManager, executor: ActionExecutor):
        super().__init__()
        self.config = config
        self.executor = executor

        self.title("Disarray Desnarler")
        self.geometry("440x420")
        self.resizable(False, False)

        self._build_header()
        self._build_grid()
        self._build_footer()

    def _build_header(self):
        row = ctk.CTkFrame(self, fg_color="transparent")
        row.pack(fill="x", padx=20, pady=(16, 4))

        ctk.CTkLabel(row, text="Disarray Desnarler",
                     font=ctk.CTkFont(size=20, weight="bold")).pack(side="left")

        self.status_label = ctk.CTkLabel(row, text="● Listening",
                                         text_color="#4CAF50",
                                         font=ctk.CTkFont(size=12))
        self.status_label.pack(side="right", pady=2)

    def _build_grid(self):
        grid = ctk.CTkFrame(self, fg_color="transparent")
        grid.pack(padx=20, pady=12)

        self.cards: dict[str, KeyCard] = {}
        for i, (key, pos) in enumerate(zip(KEY_NAMES, KEY_POSITIONS)):
            row, col = divmod(i, 2)
            card = KeyCard(
                grid, key, pos, self.config,
                on_edit=self._open_edit,
                on_test=self._test_key,
                width=172, height=140,
            )
            card.grid(row=row, column=col, padx=8, pady=8)
            self.cards[key] = card

    def _build_footer(self):
        ctk.CTkLabel(
            self,
            text="Keys map to F13 – F16 in your QMK firmware.",
            font=ctk.CTkFont(size=11),
            text_color="gray",
        ).pack(pady=(0, 14))

    def _open_edit(self, key_name: str):
        EditDialog(self, key_name, self.config, self._on_key_saved)

    def _on_key_saved(self, key_name: str):
        self.cards[key_name].refresh()

    def _test_key(self, key_name: str):
        action = self.config.get_action(key_name)
        self.executor.run(action)

    def set_status(self, text: str, color: str = "#4CAF50"):
        self.status_label.configure(text=text, text_color=color)
