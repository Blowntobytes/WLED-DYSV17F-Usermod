#pragma once
#include "wled.h"

/*
 * WLED usermod "MP3 Sound Module": serial MP3 / voice playback module driver.
 *
 * Supports two modules, selectable on the Settings > Usermods page:
 *   - DY-SV17F  (0xAA framed protocol, checksum)
 *   - JQ6500    (0x7E framed protocol, no checksum)
 *
 * Target: WLED v16.0.0 (v2 usermod API).
 *
 * Enable at compile time with the build flag:
 *     -D WLED_USERMOD_DY_SV17F
 * See platformio_override.ini (this folder) and platformio_override.usermods.ini.
 *
 * Hardware / protocol:
 *   - DY-SV17F: module in UART control mode (hardware wiring, see readme.md)
 *   - Both modules: 9600 baud, 8N1
 */

#ifdef WLED_USERMOD_DY_SV17F

// Compile-time fallback UART pins (overridable with -D DYSV17F_DEFAULT_TX_PIN / _RX_PIN).
#ifndef DYSV17F_DEFAULT_TX_PIN
  #if defined(ARDUINO_ARCH_ESP32C3)
    #define DYSV17F_DEFAULT_TX_PIN 4
    #define DYSV17F_DEFAULT_RX_PIN 5
  #elif defined(ARDUINO_ARCH_ESP32)
    #define DYSV17F_DEFAULT_TX_PIN 17
    #define DYSV17F_DEFAULT_RX_PIN 16
  #else // ESP8266 (SoftwareSerial, arbitrary GPIO)
    #define DYSV17F_DEFAULT_TX_PIN 4
    #define DYSV17F_DEFAULT_RX_PIN 5
  #endif
#endif

class UsermodDY_SV17F : public Usermod {

  private:

// ---------------- module selection ----------------
    static const uint8_t MODULE_DY_SV17F = 0;
    static const uint8_t MODULE_JQ6500   = 1;

    // ---------------- DY-SV17F serial protocol (0xAA framed) ----------------
    static const uint8_t SERIAL_BAUD     = 9600;   // both modules run at 9600 8N1
    static const uint8_t  CMD_PLAY        = 0x07;   // AA 07 02 H L SUM  (play track H*256+L, 1..65535)
    static const uint8_t  CMD_VOLUME      = 0x13;   // AA 13 01 V  SUM  (set volume 0..30)
    static const uint8_t  CMD_VOLUME_UP   = 0x14;   // AA 14 00 BE       (volume up, convenience)
    static const uint8_t  CMD_VOLUME_DOWN = 0x15;   // AA 15 00 BF       (volume down, convenience)
    static const uint8_t  MAX_VOLUME      = 30;

    // ---------------- JQ6500 serial protocol (0x7E framed) ----------------
    // frame: 0x7E [LEN] [CMD] [DATA...] 0xEF, LEN = 2 + datalen, no checksum
    // (as implemented/tested by sleemanj/JQ6500_Serial)
    static const uint8_t  JQ_CMD_PLAY_IDX = 0x03;   // 7E 04 03 H L EF  (play track index, 1..N)
    static const uint8_t  JQ_CMD_VOL_SET  = 0x06;   // 7E 03 06 V  EF  (set volume 0..30)
    static const uint8_t  JQ_CMD_VOL_UP   = 0x04;   // 7E 02 04 EF      (volume up, convenience)
    static const uint8_t  JQ_CMD_VOL_DN   = 0x05;   // 7E 02 05 EF      (volume down, convenience)

    // ---------------- button handling ----------------
    static const unsigned long DEBOUNCE_MS = 40;

    // ---------------- runtime state ----------------
    bool enabled = true;
    bool initDone = false;
    uint16_t lastTrack = 0;

    // ---------------- software debounce state ----------------
    bool lastButtonState = HIGH;
    bool debouncedState = HIGH;
    unsigned long lastDebounceTime = 0;

    // ---------------- persistent config (cfg.json -> "um": {"MP3 Sound Module": {...}}) ----------------
    uint8_t module = MODULE_DY_SV17F;               // 0 = DY-SV17F, 1 = JQ6500
    uint8_t volume = 25;
    uint16_t numSounds = 9;
    bool randomMode = false;
    int8_t buttonPin = -1;                          // -1 = button disabled until configured
    int8_t txPin = DYSV17F_DEFAULT_TX_PIN;
    int8_t rxPin = DYSV17F_DEFAULT_RX_PIN;

    // strings reused more than once (saves flash)
    static const char _name[];
    static const char _enabled[];
    static const char _module[];
    static const char _volume[];
    static const char _numSounds[];
    static const char _randomMode[];
    static const char _buttonPin[];
    static const char _txPin[];
    static const char _rxPin[];

    // ---------------- helpers ----------------
    bool initUART();
    void sendCmd(uint8_t cmd, uint8_t len, const uint8_t* data);
    void playTrack(uint16_t track);
    void setVolume(uint8_t vol);
    void volumeUp();
    void volumeDown();
    void seedRandom();
    void handleButton();
    void triggerPlay();

  public:

    void setup() override;
    void loop() override;
    void addToJsonInfo(JsonObject& root) override;
    void addToConfig(JsonObject& root) override;
    bool readFromConfig(JsonObject& root) override;
    void appendConfigData() override;
};

#endif // WLED_USERMOD_DY_SV17F