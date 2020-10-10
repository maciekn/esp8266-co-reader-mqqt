#include <SoftwareSerial.h>
#include "coreader.h"

int tx_pin = D1;
int rx_pin = D2;
SoftwareSerial InputSerial(rx_pin, tx_pin);

void setup() {
    pinMode(rx_pin, INPUT_PULLUP);
    pinMode(tx_pin, INPUT_PULLUP);
    Serial.begin(9600);
    while (!Serial)
        ;
    InputSerial.begin(9600);
    while (!InputSerial)
        ;
}


ushort buffer[300];

CoReader coReader = CoReader(InputSerial, Serial);

void loop() {
    int len = coReader.readTo(buffer, 300);

    if (len > 0) {
        Serial.println("fetched data:");
        for (int i = 0; i < len; i++) {
            Serial.print(buffer[i], HEX);
        }
        Serial.println("=============");
    }
}
