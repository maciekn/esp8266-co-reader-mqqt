#ifndef INPUTDECODER_H_
#define INPUTDECODER_H_

#include <Arduino.h>

#include <map>

#include "coreader.h"

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

    void registerCustomHandler(ushort id,
                               boolean (*handler)(ushort identifier,
                                                  ushort value, T& payload)) {
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
#endif /* INPUTDECODER_H_ */