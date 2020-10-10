#ifndef COREADER_H_
#define COREADER_H_

#include <Arduino.h>

enum READ_STATE { NONE, ENTRY, CRC, AFTER_CRC };

class CoReader {
    public:
        CoReader(Stream& serial, Stream& outstream): in_serial(serial), debug_serial(outstream) {}
        int readTo(ushort *arr, int maxSize);
    private:
        Stream& in_serial;
        Stream& debug_serial;
        ushort crc16_cycle(ushort crc, uint8_t byte);
        ushort crc16_mcrf4xx(ushort *frame, int len);
        int readNimbles();
};

#endif /* COREADER_H_ */