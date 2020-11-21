#include "dataparser.h"

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

Parser::Parser(InputDecoder<Payload> _decoder) : decoder(_decoder) {
    decoder.registerCustomHandler(collector_temp_id, handleCollectorTemp);
    decoder.registerCustomHandler(timestamp_id, handleHour);
    decoder.registerCustomHandler(water_temp_id, handleWaterTemp);
}

int Parser::serve(Payload& data) {
    return decoder.serve(data);
}