#include "coreader.h"

#include <Arduino.h>

ushort CoReader::crc16_cycle(ushort crc, uint8_t byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x0001)
            crc = (crc >> 1) ^ 0x8408;
        else
            crc = (crc >> 1);
    }
    return crc;
}

ushort CoReader::crc16_mcrf4xx(ushort *frame, int len) {
    ushort crc = 0xFFFF;

    for (int i = 0; i < len; i++) {
        // MSB
        crc = crc16_cycle(crc, (frame[i] & 0xFF00) >> 8);
        // LSB
        crc = crc16_cycle(crc, (frame[i] & 0xFF));
    }
    return crc;
};

int CoReader::readNimbles() {
    uint8_t buf[2];
    if (in_serial.readBytes(buf, 2) != 2) {
        yield();
        return -1;
    }
    ushort result = buf[0];
    result <<= 8;
    result |= buf[1];
    yield();
    return result;
}

int CoReader::readTo(ushort *arr, int maxSize) {
    READ_STATE currentState = NONE;
    int pos = 0;
    int inChar;

    if (in_serial.available()) {
        while (currentState != AFTER_CRC) {
            inChar = readNimbles();
            if (inChar == -1) {
                // timeout occured, exiting
                return -1;
            }
            if (pos == maxSize) {
                debug_serial.println("Buffer is full, too long packet! Breaking!");
                return -2;
            }
            if (currentState == NONE) {
                if (inChar != 0x0226) {
                    continue;
                }
                debug_serial.println("Moving to ENTRY state");
                currentState = ENTRY;
            } else if (currentState == ENTRY) {
                if (inChar == 0x218) {
                    debug_serial.println("Moving to CRC state");
                    currentState = CRC;
                }
            } else if (currentState == CRC) {
                ushort calculated = crc16_mcrf4xx(arr, pos - 1);
                if (calculated != inChar) {
                    debug_serial.println("Wrong CRC, dropping package!");
                    return 0;
                }
                currentState = AFTER_CRC;
            }
            arr[pos++] = inChar;
        }
        return pos;
    }
    return 0;
}