# WLED usermod: DY-SV17F serial MP3 / voice module

This **v2 usermod** for [WLED](https://github.com/wled/WLED) (v16.0.0) plays sound
effects from a **DY-SV17F** voice/MP3 module, triggered by a physical push button.

* Sequential cycling `1 → 2 → … → N → 1` or **random** track selection
  (random mode avoids immediate repeats where possible)
* Configurable **volume**, **track count**, **button GPIO** and **UART pins**
  on *Settings → Usermods* (persisted in `cfg.json`)
* Non-blocking software-debounced button, `INPUT_PULLUP`
* Works on **ESP32**, **ESP32-C3** (and S2/S3) and **ESP8266**

---

## 1. How it works

The usermod connects to the DY-SV17F module over **UART at 9600 baud, 8N1** and
sends `0xAA`-framed commands to set the volume and to play a specific track.
Every press of the button plays the next (or a random) sound effect.

The module never has to send anything back for these commands — the usermod only
writes to the UART — so it also works fine (and gracefully) if the module is not
connected at all.

| Hardware | UART used                                   | Notes |
|----------|---------------------------------------------|-------|
| ESP32 / C3 / S2 / S3 | `HardwareSerial(1)` (UART1) | Pins freely remappable to any GPIO, including on the ESP32-C3 |
| ESP8266 | `SoftwareSerial` (bundled with the core) | Any GPIO; RX optional (only TX is used) |

---

## 2. Wiring

### 2.1 DY-SV17F → UART mode (hardware)

The DY-SV17F has three onboard pin-header options: **CON1** (UART), **CON2**
(ADKEY / parallel key) and **CON3** (USB / factory). To talk to it over UART the
module must be physically placed in **UART mode** — this is a hardware wiring
step, *not* something the firmware can change.

> Depending on the board revision, UART mode is selected by the position of
> onboard **jumpers/solder bridges** or by **what is connected to CON1**.
> Check the sticker silkscreen next to the headers: CON1 = `RX`, `TX`, `VCC`,
> `GND`. Do not plug anything into CON2/CON3 (or bridge the pads) unless you
> specifically want ADKEY/factory modes.

### 2.2 ESP32 ↔ DY-SV17F (UART, TX/RX crossed)

```
DY-SV17F (CON1)          ESP32 (GPIO)
---------------          ------------
RX      <------------    GPIO4   (txPin, default)  3.3V logic
TX      ------------>    GPIO5   (rxPin, default, optional for these commands)
VCC     ------------     VIN / 5V (module takes 3.7-5V, has own 5W amp)
GND     ------------     GND   (COMMON GROUND - mandatory)
```

* **TX/RX must be crossed**: ESP32 `txPin` → module `RX`, module `TX` → ESP32 `rxPin`.
* **Common GND** between the ESP32 and the module is required.
* The DY-SV17F is 3.3V-logic compatible on CON1 (it is designed to be driven by
  MCUs), so no level shifter is needed for the ESP32.
* Keep the wires short; route the speaker away from the antenna if possible.
* On the **ESP8266** any two free GPIOs can be used via SoftwareSerial; on the
  ESP32/C3 the `txPin`/`rxPin` config is required because those UARTs are
  software-remappable (there is no fixed second UART pin mapping).

### 2.3 Push button

```
ESP32                     button
-----                     ------
GPIO (buttonPin)  ---+----(switch)----+--- GND
                     |                |
                     +---[ 10k ]------+
                     (optional external pull-up; INPUT_PULLUP is enabled in
                      firmware so a simple momentary switch to GND suffices)
```

* A momentary push button between `buttonPin` and **GND**.
* The pin is configured with `INPUT_PULLUP` (active-low).
* Debounced in software (40 ms); the next press only registers after release.
* `buttonPin = -1` disables the button.

### 2.4 Default pins (configurable, see below)

| Pin | ESP32 | ESP32-C3 | ESP8266 |
|-----|-------|----------|---------|
| `txPin` (→ module RX) | 17 | 4 | 4 |
| `rxPin` (← module TX) | 16 | 5 | 5 |
| `buttonPin` | -1 (unset) | -1 (unset) | -1 (unset) |

---

## 3. Installing the usermod

1. Get WLED **v16.0.0**:
   ```
   git clone --branch v16.0.0 --depth 1 https://github.com/wled/WLED.git
   cd WLED
   ```
2. Copy the usermod into the repo:
   ```
   cp -r <this-repo>/usermods/dy_sv17f usermods/
   ```
3. Copy the build override to the project root (or merge its `[env]` sections
   into your own `platformio_override.ini`):
   ```
   cp usermods/platformio_override.usermods.ini platformio_override.ini
   ```
4. Build (choose the environment for your board):
   ```
   pio run -e usermods_esp32c3     # ESP32-C3
   pio run -e usermods_esp32       # generic ESP32
   pio run -e usermods_esp32s3     # ESP32-S3
   pio run -e usermods_esp32s2     # ESP32-S2
   ```
   (For ESP8266, use the `[env:nodemcuv2_usermod_dy_sv17f]` section from
   `usermods/dy_sv17f/platformio_override.ini`.)

**Compile-time enable:** the usermod is compiled only when the macro
`WLED_USERMOD_DY_SV17F` is defined (the overrides above add `-D WLED_USERMOD_DY_SV17F`
automatically). To disable it, simply remove that flag and the usermod is left
out of the build.

---

## 4. Configuration (Settings → Usermods)

The values are stored in `cfg.json` under `"um": {"dy_sv17f": {...}}` via
`addToConfig()` / `readFromConfig()` and are edited on the
*Settings → Usermods* page.

| Setting       | Type  | Range/Default          | Meaning |
|---------------|-------|------------------------|---------|
| `enabled`     | bool  | `true`                 | Master switch for the usermod |
| `volume`      | int   | 0–30, default `25`     | Volume, sent to the module **on boot** and **immediately when changed** |
| `numSounds`   | int   | ≥ 1, default `9`       | Number of sound effects (track count) to cycle through |
| `randomMode`  | bool  | `false`                | `true` = random track per press (no immediate repeats), `false` = sequential `1..N..1` |
| `buttonPin`   | pin   | `-1` (disabled)        | GPIO of the push button |
| `txPin`       | pin   | see defaults above     | UART TX GPIO → module `RX` |
| `rxPin`       | pin   | see defaults above     | UART RX GPIO ← module `TX` (optional) |

Notes:

* Pin fields get WLED's usual pin-conflict checking on the Usermods page
  (red = conflict, yellow = warning).
* `rxPin = -1` is fine: these commands only send, they never read from the module.
* If `txPin = -1` the UART (and the usermod) stays disabled, so nothing is sent —
  this is the graceful "module not connected" case.

Example `cfg.json` fragment:

```json
{
  "um": {
    "dy_sv17f": {
      "enabled": true,
      "volume": 25,
      "numSounds": 9,
      "randomMode": false,
      "buttonPin": 4,
      "txPin": 17,
      "rxPin": 16
    }
  }
}
```

---

## 5. DY-SV17F command table (as used)

Serial: **9600 baud, 8N1**. Frame: `0xAA [CMD] [LEN] [DATA...] [SUM]`
where `SUM = (0xAA + CMD + LEN + Σ DATA) & 0xFF`.

| Command               | Frame                           | Notes |
|-----------------------|---------------------------------|-------|
| Play specified track  | `AA 07 02 H L SUM`              | Track number big-endian, 1–65535 (`H` = high byte, `L` = low byte) |
| Set volume            | `AA 13 01 VOL SUM`              | `VOL` = 0–30 (max 30) |
| Volume up (optional)  | `AA 14 00 BE`                   | convenience; implemented in the usermod but not exposed in the UI |
| Volume down (optional)| `AA 15 00 BF`                   | convenience; implemented in the usermod but not exposed in the UI |

**Checksum examples**

```
Play track 1:    AA 07 02 00 01 (0xAA+0x07+0x02+0x00+0x01 = 0xB4)  -> AA 07 02 00 01 B4
Play track 256:   AA 07 02 01 00 (0xAA+0x07+0x02+0x01+0x00 = 0xB4)  -> AA 07 02 01 00 B4
Set volume 25:    AA 13 01 19 (0xAA+0x13+0x01+0x19 = 0xE7)          -> AA 13 01 19 E7
Volume up:        AA 14 00 BE
Volume down:      AA 15 00 BF
```

The DY-SV17F plays files from a directory such as `01.mp3` … `99.mp3` on its
4 MB flash. File numbering must match the track numbers you cycle through
(`numSounds` = number of files).

---

## 6. Files

```
usermods/dy_sv17f/
├── library.json                  # PlatformIO lib manifest (libArchive: false)
├── platformio_override.ini       # per-usermod build snippets (ESP32, C3, ESP8266)
├── readme.md                     # this file
├── usermod_dy_sv17f.h            # usermod class (v2 API)
└── usermod_dy_sv17f.cpp          # implementation + REGISTER_USERMOD
```

Upstream repository: <https://github.com/Blowntobytes/WLED-DYSV17F-Usermod>