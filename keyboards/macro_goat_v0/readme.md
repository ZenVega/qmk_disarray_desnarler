# macro_goat_v0

An RP2040-Zero macropad: 8 keys (2 x 4 matrix), 4 rotary encoders and a
16-LED WS2812 strip.

* Keyboard Maintainer: [zen_vega](https://github.com/zen_vega)
* Hardware Supported: Waveshare RP2040-Zero
* USB IDs: VID `0x5A56`, PID `0x4D47`

Build and flash the default keymap (after setting up your build environment):

    qmk compile -kb macro_goat_v0 -km default
    qmk flash   -kb macro_goat_v0 -km default

`qmk flash` waits until the board's `RPI-RP2` drive is mounted. If your desktop
doesn't automount removable drives, mount it yourself
(`udisksctl mount -b /dev/sdX1`) or run an automounter such as `udiskie`.

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down the top left key and plug in the keyboard
* **BOOT button**: Hold BOOT on the RP2040-Zero while plugging it in (or hold
  BOOT and tap RESET)
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available

## VIA

The default keymap has VIA enabled, so keys and encoders can be remapped live
at [usevia.app](https://usevia.app) without reflashing. The board isn't in
VIA's definition database, so VIA needs the definition file from this repo:

1. Flash the default keymap.
2. In VIA, open **Settings** and enable **Show Design tab**.
3. In the **Design** tab, click **Load** and pick
   [`keymaps/default/via.json`](keymaps/default/via.json).
4. Switch to **Configure** — the 8 keys and 4 encoders are now editable.

VIA remembers the definition per browser, so step 2–3 is a one-time thing.

VIA stores the keymap in the board's EEPROM. Once it has booted with VIA,
changes to `keymap.c` won't show up until the EEPROM is reset: hold the top
left key while plugging in (Bootmagic reset) or use **Reset keymap** in VIA.

The lighting menu works, but `keymap.c` sets its own startup animation and
per-keypress colours, so lighting settings made in VIA are overridden on boot.

### Linux: "does not seem to respond like a VIA-enabled keyboard"

The browser needs read/write access to the board's `/dev/hidraw*` devices.
Add a udev rule (the `70-` prefix matters: it has to sort before systemd's
`73-seat-late.rules`, or the access is never granted):

    echo 'KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="5a56", ATTRS{idProduct}=="4d47", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/70-macro-goat-via.rules
    sudo udevadm control --reload-rules && sudo udevadm trigger

Then replug the board and reload VIA.
