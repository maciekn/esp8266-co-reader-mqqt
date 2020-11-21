#ifndef INPUTANALYZER_H_
#define INPUTANALYZER_H_

#include "inputdecoder.h"

const ushort collector_temp_id = 0x1701;  // alternative: 0x1742?
const ushort water_temp_id = 0x1702;      // alternative: 0x1743?
const ushort timestamp_id = 0x1620;
const ushort sth_else_id = 0x1B98;

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

class InputAnalyzer {
    private:
    InputDecoder<Payload> decoder;
    public:
    InputAnalyzer(InputDecoder<Payload> _decoder);
    int serve(Payload& data);
};


#endif