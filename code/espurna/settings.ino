/*

SETTINGS MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "Embedis.h"
#include <EEPROM.h>
#include "spi_flash.h"
#include <StreamString.h>

#define AUTO_SAVE 1

Embedis embedis(Serial);

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------

unsigned long settingsSize() {
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
    }
    return SPI_FLASH_SEC_SIZE - pos;
}

unsigned int settingsKeyCount() {
    unsigned count = 0;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
        count ++;
    }
    return count / 2;
}

String settingsKeyName(unsigned int index) {

    String s;

    unsigned count = 0;
    unsigned stop = index * 2 + 1;
    unsigned pos = SPI_FLASH_SEC_SIZE - 1;
    while (size_t len = EEPROM.read(pos)) {
        pos = pos - len - 2;
        count++;
        if (count == stop) {
            s.reserve(len);
            for (unsigned char i = 0 ; i < len; i++) {
                s += (char) EEPROM.read(pos + i + 1);
            }
            break;
        }
    }

    return s;

}

void settingsSetup() {

    EEPROM.begin(SPI_FLASH_SEC_SIZE);

    Embedis::dictionary( F("EEPROM"),
        SPI_FLASH_SEC_SIZE,
        [](size_t pos) -> char { return EEPROM.read(pos); },
        [](size_t pos, char value) { EEPROM.write(pos, value); },
        #if AUTO_SAVE
            []() { EEPROM.commit(); }
        #else
            []() {}
        #endif
    );

    Embedis::hardware( F("WIFI"), [](Embedis* e) {
        StreamString s;
        WiFi.printDiag(s);
        e->response(s);
    }, 0);

    Embedis::command( F("RECONNECT"), [](Embedis* e) {
        wifiConfigure();
        wifiDisconnect();
        e->response(Embedis::OK);
    });

    Embedis::command( F("RESET"), [](Embedis* e) {
        e->response(Embedis::OK);
        ESP.restart();
    });

    Embedis::command( F("STATUS"), [](Embedis* e) {
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        e->response(Embedis::OK);
    });

    Embedis::command( F("EEPROM.DUMP"), [](Embedis* e) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            if (i % 16 == 0) Serial.printf("\n[%04X] ", i);
            Serial.printf("%02X ", EEPROM.read(i));
        }
        Serial.printf("\n");
        e->response(Embedis::OK);
    });

    Embedis::command( F("EEPROM.ERASE"), [](Embedis* e) {
        for (unsigned int i = 0; i < SPI_FLASH_SEC_SIZE; i++) {
            EEPROM.write(i, 0xFF);
        }
        EEPROM.commit();
        e->response(Embedis::OK);
    });

    Embedis::command( F("SETTINGS.SIZE"), [](Embedis* e) {
        e->response(String(settingsSize()));
    });

    Embedis::command( F("DUMP"), [](Embedis* e) {
        unsigned int size = settingsKeyCount();
        for (unsigned int i=0; i<size; i++) {
            String key = settingsKeyName(i);
            String value = getSetting(key);
            e->stream->printf("+%s => %s\n", key.c_str(), value.c_str());
        }
        e->response(Embedis::OK);
    });

    DEBUG_MSG("[SETTINGS] EEPROM size: %d bytes\n", SPI_FLASH_SEC_SIZE);
    DEBUG_MSG("[SETTINGS] Settings size: %d bytes\n", settingsSize());

}

void settingsLoop() {
    embedis.process();
}

template<typename T> String getSetting(const String& key, T defaultValue) {
    String value;
    if (!Embedis::get(key, value)) value = String(defaultValue);
    return value;
}

String getSetting(const String& key) {
    return getSetting(key, "");
}

template<typename T> bool setSetting(const String& key, T value) {
    return Embedis::set(key, String(value));
}

bool delSetting(const String& key) {
    return Embedis::del(key);
}

void saveSettings() {
    DEBUG_MSG("[SETTINGS] Saving\n");
    #if not AUTO_SAVE
        EEPROM.commit();
    #endif
}
