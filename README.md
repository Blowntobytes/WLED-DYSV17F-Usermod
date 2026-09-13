# WLED MP3 Sound Module Usermod (DY-SV17F / JQ6500)

Plays sound effects from a serial MP3 / voice module, triggered by a physical push
button. Two modules are supported, selected with a dropdown on the
*Settings → Usermods* page:

- **DY-SV17F** — 4 MB flash, 5 W class-D amp, `0xAA`-framed protocol (with checksum)
- **JQ6500** — `0x7E`-framed protocol (no checksum), volume 0–30

Designed for **WLED 16.0.0**. The usermod shows up on the Usermods page as
**"MP3 Sound Module"**.

```
   ┌───────────────────────────────┐
   │            ESP32              │
   │   [buttonPin] ────┐           │
   │   [txPin]  ───────┼───┐       │
   │   [rxPin]  ───────┼───┼───┐   │
   └───────────────────┼───┼───┼───┘
                       │   │   │
                     ┌─┴───┴───┴─┐
                     │  MP3 module │
                     │ (UART 9600) │
                     └─────┬─────┘
                           │
                     ┌─────┴─────┐
                     │  speaker  │
                     └───────────┘
```

## Features

- **Cycles sound effects on every button press**:
  - **sequential**: `1 → 2 → … → N → 1`
  - **random**: random track each press (no immediate repeats when possible)
- **Two MP3 modules** supported, selectable in settings:
  - **DY-SV17F** — `0xAA`-framed commands with a checksum byte
  - **JQ6500** — `0x7E`-framed commands, no checksum
- **Configurable volume (0–30)**, sent to the module on boot, when changed, and
  immediately before every play — so the module is always at the right volume.
- **Non-blocking, software-debounced button** (`INPUT_PULLUP`, triggers on press,
  requires release before the next press registers).
- **No WLED button dependency** — the usermod reads its own GPIO.
- **Graceful failure**: if the module isn't connected (or `txPin = -1`) the
  usermod simply stays quiet; the rest of WLED is unaffected.
- Works on **ESP32**, **ESP32-C3** (and S2/S3) and **ESP8266**.

## How it works

The usermod connects to the module over **UART at 9600 baud, 8N1** and sends the
module's framed commands to set the volume and to play a specific track. Every
press of the button plays the next (or a random) sound effect.

- **DY-SV17F** frames start `0xAA`: `0xAA [CMD] [LEN] [DATA...] [SUM]` where
  `SUM = (0xAA + CMD + LEN + Σ DATA) & 0xFF`.
- **JQ6500** frames start `0x7E`: `0x7E [LEN] [CMD] [DATA...] 0xEF` where
  `LEN = 2 + len(DATA)` and there is **no checksum** (the format proven by the
  `JQ6500_Serial` library).

The two transmitters are kept **fully separate** in code, so the DY-SV17F path is
independent of the JQ6500 path. The dropdown only decides which transmitter is
used. The modules never have to reply for these commands (TX-only), so `rxPin`
is optional.

## Hardware & wiring

### MP3 module ↔ ESP32 (UART, TX/RX crossed)

| Signal          | DY-SV17F (CON1) | JQ6500 | ESP32        | ESP32-C3     | ESP8266     |
|-----------------|-----------------|--------|--------------|--------------|-------------|
| Module RX       | RX              | RX     | `txPin` (17) | `txPin` (4)  | `txPin` (4) |
| Module TX       | TX              | TX     | `rxPin` (16) | `rxPin` (5)  | `rxPin` (5) |
| Power           | VCC             | VCC    | 5V / VIN     | 5V / VIN     | 5V / VIN    |
| Ground          | GND             | GND    | GND          | GND          | GND         |

- **TX/RX must be crossed**: ESP32 `txPin` → module **RX**, module **TX** →
  ESP32 `rxPin`.
- **Common GND** between the ESP32 and the module is mandatory.
- `rxPin` is optional (these commands only send), so it may be left at `-1`.
- The pins are configurable in *Settings → Usermods* (defaults in the table).
- On the **ESP8266** the usermod uses `SoftwareSerial`; on ESP32/C3/S2/S3 it uses
  the second hardware UART (`HardwareSerial(1)`), whose pins are remappable to
  almost any GPIO.

### DY-SV17F → UART mode (hardware)

The DY-SV17F has three on-board mode pads: **CON1** (UART), **CON2** (ADKEY) and
**CON3** (USB / factory). It must be physically placed in **UART mode** — this is
a hardware step, *not* something the firmware can change. UART mode is `0-0-1`
(tie each pad via a 10 kΩ resistor):

| Pad  | Tie to |
|------|--------|
| CON1 | GND    |
| CON2 | GND    |
| CON3 | 3.3 V  |

Some board revisions ship with the resistors already fitted. Once set, use the
CON1 header (`RX`, `TX`, `VCC`, `GND`) for wiring.

### Push button

```
ESP32                     button
-----                     ------
GPIO (buttonPin)  ──+───(switch)───+─── GND
                    │              │
                    └──[ 10k ]─────┘
                    (optional; INPUT_PULLUP is enabled in firmware, so a simple
                     momentary switch to GND suffices)
```

- A momentary push button between `buttonPin` and **GND**.
- The pin is configured `INPUT_PULLUP` (active-low), debounced in software
  (40 ms), and requires release before the next press registers.
- `buttonPin = -1` disables the button.

> Tip: the same button can also drive WLED's own button system (e.g. cycle
> lighting effects) — both simply read the same pin. To make *short* press do
> one thing and *long* press the other, add the press-duration logic (ask in an
> issue and I'll add it).

## Integration with WLED 16.0.0

The usermod is **compile-time optional**: it is only built when the macro
`WLED_USERMOD_DY_SV17F` is defined (the provided overrides add it), so the stock
WLED build is completely unaffected.

### Option A — use the included build override (recommended)

From a pristine WLED **v16.0.0** checkout:

```bash
git clone --branch v16.0.0 --depth 1 https://github.com/wled/WLED.git
cd WLED

# 1. copy the usermod into the repo
cp -r <this-repo>/usermods/dy_sv17f usermods/

# 2. copy the build override to the project root
cp <this-repo>/usermods/platformio_override.usermods.ini platformio_override.ini

# 3. build (pick your environment)
pio run -e usermods_esp32c3      # ESP32-C3
pio run -e usermods_esp32        # generic ESP32
pio run -e usermods_esp32s3      # ESP32-S3
pio run -e usermods_esp32s2      # ESP32-S2
```

(For ESP8266, use the `[env:nodemcuv2_usermod_dy_sv17f]` section from
`usermods/dy_sv17f/platformio_override.ini`.)

### Option B — add to your own override

Merge the `[env:usermods_*]` sections into your existing `platformio_override.ini`,
or simply add `dy_sv17f` to your `custom_usermods` list and add
`-D WLED_USERMOD_DY_SV17F` to the env's `build_flags`.

> The override shipped here builds **only** this usermod
> (`custom_usermods = dy_sv17f`). If you build several usermods, append
> `dy_sv17f` to your existing list instead.

## Settings

All settings live under the `MP3 Sound Module` object in `cfg.json` and are
editable on **Settings → Usermods** in the WLED web UI.

| Key          | Type   | Default           | Description |
|--------------|--------|-------------------|-------------|
| `enabled`    | bool   | `true`            | Master switch for the usermod |
| `module`     | select | `DY-SV17F`        | Which MP3 module is connected (dropdown: DY-SV17F / JQ6500) |
| `volume`     | int    | `25` (0–30)       | Volume, sent on boot, when changed, and before every play |
| `numSounds`  | int    | `9`               | Number of sound effects (track count) to cycle through |
| `randomMode` | bool   | `false`           | `true` = random track per press, `false` = sequential `1..N..1` |
| `buttonPin`  | pin    | `-1` (disabled)   | GPIO of the push button |
| `txPin`      | pin    | see wiring table  | UART TX GPIO → module `RX`; `-1` disables the UART |
| `rxPin`      | pin    | see wiring table  | UART RX GPIO ← module `TX` (optional) |

Pin fields get WLED's usual pin-conflict checking on the Usermods page.

## Command tables

### DY-SV17F — `0xAA [CMD] [LEN] [DATA...] [SUM]`, `SUM = (0xAA + CMD + LEN + Σ DATA) & 0xFF`

| Command               | Frame                     | Notes |
|-----------------------|---------------------------|-------|
| Play specified track  | `AA 07 02 H L SUM`        | Track number big-endian, 1–65535 |
| Set volume            | `AA 13 01 VOL SUM`        | `VOL` = 0–30 (max 30) |
| Volume up (optional)  | `AA 14 00 BE`             | convenience, not exposed in the UI |
| Volume down (optional)| `AA 15 00 BF`             | convenience, not exposed in the UI |

### JQ6500 — `0x7E [LEN] [CMD] [DATA...] 0xEF`, `LEN = 2 + len(DATA)`, no checksum

| Command               | Frame              | Notes |
|-----------------------|--------------------|-------|
| Play track by index   | `7E 04 03 H L EF`  | Track index big-endian, 1-based |
| Set volume            | `7E 03 06 VOL EF`  | `VOL` = 0–30 (max 30) |
| Volume up (optional)  | `7E 02 04 EF`      | convenience, not exposed in the UI |
| Volume down (optional)| `7E 02 05 EF`      | convenience, not exposed in the UI |

Audio files are loaded over each module's **USB port** (they appear as a drive
on a PC). DY-SV17F files are named `01.mp3`, `02.mp3`, …; JQ6500 tracks are
indexed by their order in the drive's file table, so copy them in play order.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Button does nothing (track counter in `/json/info` does not change) | Check `buttonPin` matches the physically wired GPIO; reboot after pin changes |
| Track counter changes but no sound | Module not in UART mode (DY-SV17F `0-0-1`), TX/RX swapped, no common GND, wrong `module` selected, or files not loaded on the module |
| Sound plays but is quiet | Raise `volume` to 30; the module's amp needs a bigger speaker (3–5 W) than a tiny 0.5 W one |
| Module silent after boot | Volume is re-sent before every play, so just press the button again; check `txPin` |
| Serial monitor shows nothing | `txPin`/`rxPin` not set, or `enabled` off |

## Files

```
usermods/dy_sv17f/
├── library.json                  # PlatformIO library manifest (libArchive: false)
├── platformio_override.ini       # per-usermod build snippets (ESP32, C3, ESP8266)
├── readme.md                     # in-tree readme (wiring, config, command tables)
├── usermod_dy_sv17f.h            # usermod class (v2 API)
└── usermod_dy_sv17f.cpp          # implementation + REGISTER_USERMOD
usermods/
├── readme.md                     # updated usermods listing
└── platformio_override.usermods.ini  # updated build config (adds dy_sv17f)
```

## License

MIT — see [LICENSE](LICENSE). This usermod is not part of the official WLED
project.