#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <SoftwareSerial.h>
#include <ThingSpeak.h>
#include <WiFiManager.h>

#include "coreader.h"

int tx_pin = D1;
int rx_pin = D2;
SoftwareSerial InputSerial(rx_pin, tx_pin);

const ushort collector_temp_id = 0x1701;  // alternative: 0x1742?
const ushort water_temp_id = 0x1702;      // alternative: 0x1743?
const ushort timestamp_id = 0x1620;
const ushort sth_else_id = 0x1B98;

const char CONFIG_LOCATION[] = "/config.json";

char myChannelNumber[20];
char myWriteAPIKey[34];

WiFiClient espClient;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

bool shouldSaveConfig;

void saveConfigCallback() {
    shouldSaveConfig = true;
}

void loadStoredData() {
    Serial.println("Opening file");
    if (LittleFS.exists(CONFIG_LOCATION)) {


        File configFile = LittleFS.open(CONFIG_LOCATION, "r");
        Serial.println("File opened");
        if (configFile) {
            DynamicJsonDocument jsonBuffer(1024);
            Serial.println("Content loaded");
            if (deserializeJson(jsonBuffer, configFile) ==
                DeserializationError::Ok) {
                JsonObject json = jsonBuffer.as<JsonObject>();
                strcpy(myChannelNumber, json["channel"]);
                strcpy(myWriteAPIKey, json["apiKey"]);
            } else {
                Serial.println("failed to load json config");
            }
            configFile.close();
        }
    }
}

void initalizeWiFiManager() {
    WiFiManagerParameter custom_channel_no("channel", "MQQT Channel",
                                           myChannelNumber, 20);
    WiFiManagerParameter custom_api_key("apiKey", "API Key", myWriteAPIKey, 32);

    WiFiManager wifiManager;

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_channel_no);
    wifiManager.addParameter(&custom_api_key);

    wifiManager.autoConnect("AutoConnectAP");
    
    delay(500);

    strcpy(myChannelNumber, custom_channel_no.getValue());
    strcpy(myWriteAPIKey, custom_api_key.getValue());
}

void cleanData() {
    Serial.println("Clearing settings!");
    LittleFS.remove(CONFIG_LOCATION);
    WiFiManager wifiManager;
    wifiManager.resetSettings();
}

void setup() {
    pinMode(rx_pin, INPUT_PULLUP);
    pinMode(tx_pin, INPUT_PULLUP);
    pinMode(0, INPUT_PULLUP);

    Serial.begin(9600);
    while (!Serial);
    InputSerial.begin(9600);
    while (!InputSerial);

    LittleFS.begin();

    Serial.println("Welcome to the MQQT jungle!");
    Serial.println("Press flash button now to reset settings");
    delay(2000);
    if (digitalRead(0) == 0) {
        cleanData();
    }

    loadStoredData();

    initalizeWiFiManager();

    if (shouldSaveConfig) {
        Serial.println("saving config");
        DynamicJsonDocument jsonBuffer(1024);
        JsonObject json = jsonBuffer.to<JsonObject>();
        json["channel"] = myChannelNumber;
        json["apiKey"] = myWriteAPIKey;

        File configFile = LittleFS.open(CONFIG_LOCATION, "w");
        if (!configFile) {
            Serial.println("failed to open config file for writing");
        }
        serializeJson(jsonBuffer, configFile);
        configFile.close();
    }

    ThingSpeak.begin(espClient);

    httpUpdater.setup(&server);

	server.begin();

}

struct Payload {
    ushort collector_temp = 0;
    ushort water_temp = 0;
    ushort hour = 0;
    ushort min = 0;
};

int decodeInput(ushort* buffer, int len, Payload* dest) {
    int noOfValues = 0;
    for (int i = 0; i < (len - 1); i += 2) {
        switch (buffer[i]) {
            case collector_temp_id:
                dest->collector_temp = buffer[i + 1];
                noOfValues++;
                break;
            case water_temp_id:
                dest->water_temp = buffer[i + 1];
                noOfValues++;
                break;
            case timestamp_id:
                dest->hour = (buffer[i + 1] & 0xFF00) >> 8;
                dest->min = buffer[i + 1] & 0xFF;
                noOfValues++;
                break;
        }
    }
    return noOfValues;
}

ushort buffer[300];

CoReader coReader = CoReader(Serial, Serial);

void loop() {
    int len = coReader.readTo(buffer, 300);

    if (len > 0) {
        Payload data;
        if (decodeInput(buffer, len, &data)) {
            Serial.println("Sending data to ThingSpeak");
            ThingSpeak.setField(1, data.water_temp);
            ThingSpeak.setField(2, data.collector_temp);
            int httpCode =
                ThingSpeak.writeFields(atol(myChannelNumber), myWriteAPIKey);
            if (httpCode == 200) {
                Serial.println("Channel write successful.");
            } else {
                Serial.println("Problem writing to channel. HTTP error code " +
                               String(httpCode));
            }
        }
    }
    server.handleClient();
}
