#ifndef INPUTANALYZER_H_
#define INPUTANALYZER_H_

#include "inputdecoder.h"

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