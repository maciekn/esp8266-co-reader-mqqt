#include <SoftwareSerial.h>

#include "coreader.h"

int tx_pin = D1;
int rx_pin = D2;
SoftwareSerial InputSerial(rx_pin, tx_pin);

const ushort collector_temp_id = 0x1701;  // alternative: 0x1742?
const ushort water_temp_id = 0x1702;      // alternative: 0x1743?
const ushort timestamp_id = 0x1620;
const ushort sth_else_id = 0x1B98;

void setup() {
    pinMode(rx_pin, INPUT_PULLUP);
    pinMode(tx_pin, INPUT_PULLUP);
    Serial.begin(9600);
    while (!Serial)
        ;
    InputSerial.begin(9600);
    while (!InputSerial)
        ;
    delay(500);
}

struct Payload {
    ushort collector_temp = 0;
    ushort water_temp = 0;
    ushort hour = 0;
    ushort min = 0;
};

void decodeInput(ushort* buffer, int len, Payload* dest) {
    for (int i = 0; i < (len - 1); i += 2) {
        switch (buffer[i]) {
            case collector_temp_id:
                dest->collector_temp = buffer[i + 1];
                break;
            case water_temp_id:
                dest->water_temp = buffer[i + 1];
                break;
            case timestamp_id:
                dest->hour = (buffer[i + 1] & 0xFF00) >> 8;
                dest->min = buffer[i + 1] & 0xFF;
                break;
        }
    }

}

ushort buffer[300];

CoReader coReader = CoReader(Serial, Serial);

void loop() {
    int len = coReader.readTo(buffer, 300);

    if (len > 0) {
        Payload data;
        decodeInput(buffer, len, &data);
        Serial.println("fetched data:");
        Serial.print("Watertemp: ");
        Serial.println(data.water_temp);
        Serial.print("Collectortemp: ");
        Serial.println(data.collector_temp);
        Serial.print("Timestamp: ");
        Serial.print(data.hour);
        Serial.print(":");
        Serial.println(data.min);
    }
}
