
#include <map>

// Arduino framework
#include <LittleFS.h>
#include <SoftwareSerial.h>

// ESP8266 details
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

// External libs
#include <ArduinoJson.h>
#include <ThingSpeak.h>
#include <WiFiManager.h>

#include "coreader.h"

// #define DEVMODE 1

int tx_pin = D1;
int rx_pin = D2;

#ifdef DEVMODE
const unsigned long SEND_THROTTLE = 1;
SoftwareSerial InputSerial(rx_pin, tx_pin);
CoReader coReader = CoReader(Serial, Serial);
#else
const unsigned long SEND_THROTTLE = 60 * 60 * 1000;  // 60 mins
SoftwareSerial InputSerial(rx_pin, tx_pin);
CoReader coReader = CoReader(InputSerial, Serial);
#endif

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

void saveConfigCallback() { shouldSaveConfig = true; }

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

template <class T>
class InputHandler {
   public:
    ushort identifier;
    boolean (*inputCallback)(ushort identifier, ushort value, T& payload);
};

template <class T>
class InputDecoder {
   private:
    CoReader& reader;
    ushort buffer[300];
    std::map<ushort, InputHandler<T>> inputHandlers;

   public:
    InputDecoder<T>(CoReader& coReader) : reader(coReader) {}

    void registerHandler(ushort id,
                         boolean (*handler)(ushort identifier, ushort value,
                                            T& payload)) {
        InputHandler<T>* hanlderCpy = new InputHandler<T>();
        hanlderCpy->identifier = id;
        hanlderCpy->inputCallback = handler;
        inputHandlers.insert(
            std::pair<ushort, InputHandler<T>>(id, *hanlderCpy));
    }

    int serve(T& data) {
        int len = reader.readTo(buffer, 300);
        int noOfValues = 0;
        if (len > 0) {
            for (int i = 0; i < (len - 1); i += 2) {
                auto it = inputHandlers.find(buffer[i]);
                if (it != inputHandlers.end()) {
                    if (it->second.inputCallback(buffer[i], buffer[i + 1],
                                                 data)) {
                        noOfValues++;
                    }
                }
            }
        }
        return noOfValues;
    }
};

struct Payload {
    short collector_temp = 0;
    short water_temp = 0;
    ushort hour = 0;
    ushort min = 0;

    void zero() {
        collector_temp = 0;
        water_temp = 0;
        hour = 0;
        min = 0;
    }
};

boolean handleWaterTemp(ushort identifier, ushort value, Payload& payload) {
    payload.water_temp = (short)value;
    return true;
}

boolean handleCollectorTemp(ushort identifier, ushort value, Payload& payload) {
    payload.collector_temp = (short)value;
    return true;
}

boolean handleHour(ushort identifier, ushort value, Payload& payload) {
    payload.hour = (value & 0xFF00) >> 8;
    payload.min = value & 0xFF;
    return true;
}

InputDecoder<Payload> decoder(coReader);

void setup() {
    pinMode(rx_pin, INPUT_PULLUP);
    pinMode(tx_pin, INPUT_PULLUP);
    pinMode(0, INPUT_PULLUP);

    Serial.begin(9600);
    while (!Serial)
        ;
    InputSerial.begin(9600);
    while (!InputSerial)
        ;

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
    decoder.registerHandler(collector_temp_id, handleCollectorTemp);
    decoder.registerHandler(timestamp_id, handleHour);
    decoder.registerHandler(water_temp_id, handleWaterTemp);
}

void uploadData(const Payload& payload) {
    Serial.println("Sending data to ThingSpeak");
    ThingSpeak.setField(1, payload.water_temp);
    ThingSpeak.setField(2, payload.collector_temp);
#ifdef DEVMODE
    Serial.print("Watertemp: ");
    Serial.println(payload.water_temp);
    Serial.print("Collector: ");
    Serial.println(payload.collector_temp);
#else
    int httpCode = ThingSpeak.writeFields(atol(myChannelNumber), myWriteAPIKey);
    if (httpCode == 200) {
        Serial.println("Channel write successful.");
    } else {
        Serial.println("Problem writing to channel. HTTP error code " +
                       String(httpCode));
    }
#endif
}

unsigned long last_sent = ULONG_MAX;

void loop() {
    Payload p;
    if (decoder.serve(p) != 0) {
        unsigned long current_timestamp = millis();
        if ((last_sent + SEND_THROTTLE) < current_timestamp ||
            last_sent > current_timestamp) {
            uploadData(p);
            last_sent = current_timestamp;
        }
    }
    server.handleClient();
}
