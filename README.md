# WLED DY-SV17F Usermod

A [WLED](https://github.com/wled/WLED) **v2 usermod** that plays sound effects
from a **DY-SV17F** serial MP3 / voice module, driven by a physical push button.

Targets WLED **v16.0.0** (the usermod is written against the v16.0.0 tag; it
should build against later releases with only minor changes).

## Features

- Cycles sound effects on every button press:
  - **sequential**: `1 → 2 → … → N → 1`
  - **random**: random track each press (no immediate repeats when possible)
- User-configurable settings on *Settings → Usermods*, persisted in `cfg.json`:
  `volume` (0–30), `numSounds`, `randomMode`, `buttonPin`, `txPin`, `rxPin`
- Sends the configured volume to the module on boot and immediately when changed
- Non-blocking software-debounced button (`INPUT_PULLUP`, triggers on press,
  requires release before the next press)
- Works on **ESP32**, **ESP32-C3** (and S2/S3) and **ESP8266**
- Uses the DY-SV17F UART protocol at **9600 baud, 8N1**
  (`0xAA [CMD] [LEN] [DATA...] [SUM]`)

## Repository layout

This repository is a drop-in distribution of the usermod. The `usermods/`
folder mirrors the WLED repository layout:

```
usermods/
├── readme.md                            # updated usermods listing (adds DY-SV17F)
├── platformio_override.usermods.ini     # updated build config (adds dy_sv17f)
└── dy_sv17f/
    ├── library.json                     # PlatformIO lib manifest (libArchive: false)
    ├── platformio_override.ini          # per-usermod build snippets
    ├── readme.md                        # wiring, config & command table
    ├── usermod_dy_sv17f.h               # usermod class (v2 API)
    └── usermod_dy_sv17f.cpp             # implementation + REGISTER_USERMOD
```

## Installation

```bash
git clone --branch v16.0.0 --depth 1 https://github.com/wled/WLED.git
cd WLED

# 1. copy the usermod into the WLED repo
cp -r <this-repo>/usermods/dy_sv17f usermods/

# 2. copy the build override to the WLED project root
cp <this-repo>/usermods/platformio_override.usermods.ini platformio_override.ini

# 3. build (pick your environment)
pio run -e usermods_esp32c3
```

The usermod is compiled only when the macro `WLED_USERMOD_DY_SV17F` is defined;
the provided build overrides add it automatically.

> The platformio_override.usermods.ini shipped here has
> `custom_usermods = dy_sv17f`, i.e. it builds **only** this usermod. Merge the
> `[env:usermods_*]` sections into your own override if you build several usermods.

## Verified build

The usermod was compiled against WLED v16.0.0 with PlatformIO Core 6.x using:

```bash
pio run -e usermods_esp32c3
```

(an ESP32-C3 environment based on `esp32-c3-devkitm-1`). It also builds for
generic ESP32 (`usermods_esp32`) and ESP8266 (`nodemcuv2_usermod_dy_sv17f`).

## Documentation

- Wiring (module CON1/UART mode, ESP32↔module TX/RX crossed + common GND, button),
  configuration values and the DY-SV17F command table:
  [usermods/dy_sv17f/readme.md](usermods/dy_sv17f/readme.md)

## License

MIT — see [LICENSE](LICENSE).