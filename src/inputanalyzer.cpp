#include "inputanalyzer.h"

const ushort collector_temp_id = 0x1701;  // alternative: 0x1742?
const ushort water_temp_id = 0x1702;      // alternative: 0x1743?
const ushort timestamp_id = 0x1620;
const ushort sth_else_id = 0x1B98;

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

InputAnalyzer::InputAnalyzer(InputDecoder<Payload> _decoder) : decoder(_decoder) {
    decoder.registerCustomHandler(collector_temp_id, handleCollectorTemp);
    decoder.registerCustomHandler(timestamp_id, handleHour);
    decoder.registerCustomHandler(water_temp_id, handleWaterTemp);
}

int InputAnalyzer::serve(Payload& data) {
    return decoder.serve(data);
}