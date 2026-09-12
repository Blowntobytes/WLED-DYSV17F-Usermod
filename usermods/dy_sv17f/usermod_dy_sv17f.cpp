// On ESP8266, SoftwareSerial must be included BEFORE wled.h: the ESP8266 core's
// SoftwareSerial relies on its bundled Delegate implementation and including it
// after wled.h yields an "incomplete type Delegate<...>" compile error.
#if !defined(ARDUINO_ARCH_ESP32)
  #include <SoftwareSerial.h>
#endif
#include "usermod_dy_sv17f.h"

#ifdef WLED_USERMOD_DY_SV17F

#if defined(ARDUINO_ARCH_ESP32)
  #include <HardwareSerial.h>  // already pulled in via wled.h, kept for clarity
  // Second hardware UART. On ESP32 (incl. ESP32-C3/S2/S3) the pins of the
  // peripheral UARTs are freely remappable to almost any GPIO.
  static HardwareSerial dysv17f_serial(1);
#else
  // ESP8266: UART1 exposes TX only (fixed GPIO2), so use SoftwareSerial for
  // arbitrary pins. The module only needs to be *sent to* for the commands
  // implemented here, so RX is optional (rxPin may be -1).
  static SoftwareSerial* dysv17f_serial = nullptr;
#endif

const char UsermodDY_SV17F::_name[]        PROGMEM = "MP3 Sound Module";
const char UsermodDY_SV17F::_enabled[]     PROGMEM = "enabled";
const char UsermodDY_SV17F::_module[]      PROGMEM = "module";
const char UsermodDY_SV17F::_volume[]      PROGMEM = "volume";
const char UsermodDY_SV17F::_numSounds[]   PROGMEM = "numSounds";
const char UsermodDY_SV17F::_randomMode[]  PROGMEM = "randomMode";
const char UsermodDY_SV17F::_buttonPin[]   PROGMEM = "buttonPin";
const char UsermodDY_SV17F::_txPin[]       PROGMEM = "txPin";
const char UsermodDY_SV17F::_rxPin[]       PROGMEM = "rxPin";


// ============================ setup / loop ============================

bool UsermodDY_SV17F::initUART() {
#if defined(ARDUINO_ARCH_ESP32)
  // HardwareSerial::begin(baud, config, rxPin, txPin)
  if (txPin >= 0 || rxPin >= 0) {
    dysv17f_serial.begin(SERIAL_BAUD, SERIAL_8N1, rxPin, txPin);
    return true;
  }
#else
  if (txPin >= 0) {
    if (dysv17f_serial) delete dysv17f_serial;
    dysv17f_serial = new SoftwareSerial(rxPin, txPin);
    dysv17f_serial->begin(SERIAL_BAUD);
    return true;
  }
#endif
  return false;
}

void UsermodDY_SV17F::setup() {
  initDone = initUART();

  if (initDone) {
    seedRandom();
    setVolume(volume); // push the configured volume to the module on boot
    DEBUG_PRINTLN(F("[DY-SV17F] UART initialized, volume set"));
  } else {
    DEBUG_PRINTLN(F("[DY-SV17F] UART not initialized (txPin not set)"));
  }

  if (buttonPin >= 0) {
    pinMode(buttonPin, INPUT_PULLUP);
    lastButtonState = debouncedState = digitalRead(buttonPin);
  }
}

void UsermodDY_SV17F::loop() {
  if (!enabled) return;
  if (buttonPin >= 0) handleButton();
}


// ============================ button handling ============================

/*
 * Software debounce (no delay()), trigger on press (active low, INPUT_PULLUP).
 * Because triggering only happens on the debounced LOW->HIGH edge in the state
 * machine, the button must be released before the next press registers.
 */
void UsermodDY_SV17F::handleButton() {
  bool reading = digitalRead(buttonPin);
  unsigned long now = millis();

  if (reading != lastButtonState) {
    lastDebounceTime = now;
    lastButtonState = reading;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_MS && reading != debouncedState) {
    debouncedState = reading;
    if (debouncedState == LOW && initDone) triggerPlay(); // press
  }
}

void UsermodDY_SV17F::triggerPlay() {
  if (numSounds < 1) return;

  if (randomMode) {
    // random track, avoid immediate repeats where possible
    if (numSounds > 1) {
      uint16_t t;
      do {
        t = (uint16_t)random(1, numSounds + 1);
      } while (t == lastTrack);
      lastTrack = t;
    } else {
      lastTrack = 1;
    }
  } else {
    // sequential: 1 -> 2 -> ... -> N -> 1 ...
    lastTrack = (uint16_t)((lastTrack % numSounds) + 1);
  }

  playTrack(lastTrack);
}


// ============================ DY-SV17F protocol ============================

void UsermodDY_SV17F::sendCmd(uint8_t cmd, uint8_t len, const uint8_t* data) {
  if (!initDone) return;

#if defined(ARDUINO_ARCH_ESP32)
  Stream& s = dysv17f_serial;
#else
  if (!dysv17f_serial) return;
  Stream& s = *dysv17f_serial;
#endif

  if (module == MODULE_JQ6500) {
    // JQ6500 frame: 0x7E [LEN] [CMD] [DATA...] 0xEF
    // LEN = CMD(1) + DATA(n) + EF(1) = 2 + n ; no checksum
    uint8_t frame[8];
    frame[0] = 0x7E;
    frame[1] = 2 + len;
    frame[2] = cmd;
    for (uint8_t i = 0; i < len; i++) frame[3 + i] = data[i];
    frame[3 + len] = 0xEF;
    s.write(frame, 3 + len + 1);
  } else {
    // DY-SV17F frame: 0xAA [CMD] [LEN] [DATA...] [SUM]
    // SUM = (0xAA + CMD + LEN + sum(DATA)) & 0xFF
    uint8_t frame[6]; // largest frame used here: AA CMD LEN DATA(2) SUM
    frame[0] = 0xAA;
    frame[1] = cmd;
    frame[2] = len;
    uint8_t sum = 0xAA + cmd + len;
    for (uint8_t i = 0; i < len; i++) {
      frame[3 + i] = data[i];
      sum += data[i];
    }
    frame[3 + len] = sum & 0xFF;
    s.write(frame, 3 + len + 1);
  }
}

void UsermodDY_SV17F::playTrack(uint16_t track) {
  if (track < 1) track = 1;
  uint8_t data[2] = { (uint8_t)(track >> 8), (uint8_t)(track & 0xFF) }; // big-endian
  sendCmd((module == MODULE_JQ6500) ? JQ_CMD_PLAY_IDX : CMD_PLAY, 2, data);
}

void UsermodDY_SV17F::setVolume(uint8_t vol) {
  if (vol > MAX_VOLUME) vol = MAX_VOLUME;
  uint8_t data[1] = { vol };
  sendCmd((module == MODULE_JQ6500) ? JQ_CMD_VOL_SET : CMD_VOLUME, 1, data);
}

void UsermodDY_SV17F::volumeUp() {
  sendCmd((module == MODULE_JQ6500) ? JQ_CMD_VOL_UP : CMD_VOLUME_UP, 0, nullptr);
}

void UsermodDY_SV17F::volumeDown() {
  sendCmd((module == MODULE_JQ6500) ? JQ_CMD_VOL_DN : CMD_VOLUME_DOWN, 0, nullptr);
}

void UsermodDY_SV17F::seedRandom() {
#if defined(ARDUINO_ARCH_ESP32)
  randomSeed(esp_random());
#else
  randomSeed(analogRead(A0) ^ (millis() & 0xFFFF));
#endif
}


// ============================ JSON info ============================

void UsermodDY_SV17F::addToJsonInfo(JsonObject& root) {
  JsonObject user = root["u"];
  if (user.isNull()) user = root.createNestedObject("u");

  JsonArray mod = user.createNestedArray(FPSTR(_module));
  mod.add(module == MODULE_JQ6500 ? F("JQ6500") : F("DY-SV17F"));

  JsonArray trk = user.createNestedArray(FPSTR(_name));
  trk.add(lastTrack);             // current/last requested track
  trk.add(F("track"));

  JsonArray vol = user.createNestedArray(FPSTR(_volume));
  vol.add(volume);
  vol.add(F("volume"));
}


// ============================ config persistence ============================

void UsermodDY_SV17F::addToConfig(JsonObject& root) {
  JsonObject top = root.createNestedObject(FPSTR(_name));
  top[FPSTR(_enabled)] = enabled;
  top[FPSTR(_module)] = module;
  top[FPSTR(_volume)] = volume;
  top[FPSTR(_numSounds)] = numSounds;
  top[FPSTR(_randomMode)] = randomMode;
  top[FPSTR(_buttonPin)] = buttonPin;
  top[FPSTR(_txPin)] = txPin;
  top[FPSTR(_rxPin)] = rxPin;
}

bool UsermodDY_SV17F::readFromConfig(JsonObject& root) {
  JsonObject top = root[FPSTR(_name)];  // "MP3 Sound Module"
  bool configComplete = !top.isNull();

  // migrate the config that v1.0.0 saved under the legacy "dy_sv17f" key so an
  // upgrade does not silently reset the user's settings. Returning false makes
  // WLED persist the migrated values under the new key and drop the old one.
  if (top.isNull()) {
    JsonObject legacy = root["dy_sv17f"];
    if (!legacy.isNull()) {
      top = legacy;
      configComplete = false;
      root.remove("dy_sv17f");
    }
  }

  uint8_t oldVolume = volume;
  uint8_t oldModule = module;
  int8_t oldButtonPin = buttonPin;
  int8_t oldTxPin = txPin;
  int8_t oldRxPin = rxPin;

  configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled, true);
  configComplete &= getJsonValue(top[FPSTR(_module)], module, MODULE_DY_SV17F);
  configComplete &= getJsonValue(top[FPSTR(_volume)], volume, 25);
  configComplete &= getJsonValue(top[FPSTR(_numSounds)], numSounds, 9);
  configComplete &= getJsonValue(top[FPSTR(_randomMode)], randomMode, false);
  configComplete &= getJsonValue(top[FPSTR(_buttonPin)], buttonPin, -1);
  configComplete &= getJsonValue(top[FPSTR(_txPin)], txPin, DYSV17F_DEFAULT_TX_PIN);
  configComplete &= getJsonValue(top[FPSTR(_rxPin)], rxPin, DYSV17F_DEFAULT_RX_PIN);

  // readFromConfig is called at boot (before setup()) and again after every
  // save on the Settings > Usermods page. In the latter case re-apply any
  // hardware-affecting change immediately so a reboot is not required.
  if (initDone) {
    if (txPin != oldTxPin || rxPin != oldRxPin) {
      // UART pins changed: (re)start the UART on the new pins
#if defined(ARDUINO_ARCH_ESP32)
      dysv17f_serial.end(); // no-op if never begun
#endif
      initDone = initUART();
      if (initDone) setVolume(volume);
    } else if (volume != oldVolume || module != oldModule) {
      // volume or protocol changed: push the (new) volume immediately
      setVolume(volume);
    }

    if (buttonPin != oldButtonPin) {
      // button pin changed: (re)configure it
      if (buttonPin >= 0) {
        pinMode(buttonPin, INPUT_PULLUP);
        lastButtonState = debouncedState = digitalRead(buttonPin);
      }
    }
  }

  return configComplete;
}

void UsermodDY_SV17F::appendConfigData() {
  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":module"));
  oappend(F("',1,'MP3 module type connected to the UART pins.');"));

  // dropdown for module selection
  oappend(F("dd=addDropdown('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F("','module');"));
  oappend(F("addOption(dd,'DY-SV17F',0);"));
  oappend(F("addOption(dd,'JQ6500',1);"));

  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":volume"));
  oappend(F("',1,'DY-SV17F volume (0-30). Sent to the module on boot and when changed.');"));

  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":numSounds"));
  oappend(F("',1,'Number of sound effects (track count) stored on the module.');"));

  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":buttonPin"));
  oappend(F("',1,'GPIO of the push button (INPUT_PULLUP). -1 disables the button.');"));

  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":txPin"));
  oappend(F("',1,'UART TX pin connected to the module RX. -1 disables the UART.');"));

  oappend(F("addInfo('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F(":rxPin"));
  oappend(F("',1,'UART RX pin connected to the module TX (optional for these commands).' );"));

  // dropdown for volume 0..30
  oappend(F("dd=addDropdown('"));
  oappend(String(FPSTR(_name)).c_str());
  oappend(F("','volume');"));
  for (uint8_t v = 0; v <= MAX_VOLUME; v++) {
    oappend(F("addOption(dd,'"));
    oappend(String(v).c_str());
    oappend(F("',"));
    oappend(String(v).c_str());
    oappend(F(");"));
  }
}

#endif // WLED_USERMOD_DY_SV17F

// Register with WLED's usermod manager (compiled only when the macro is set).
#ifdef WLED_USERMOD_DY_SV17F
static UsermodDY_SV17F dy_sv17f_usermod;
REGISTER_USERMOD(dy_sv17f_usermod);
#endif