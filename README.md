# Description
This simple app is intended to read from solar driver via Serial protocol and then push it to ThingSpeak. By default app sends at most one record per 1 hour (can be configured by `SEND_THROTTLE` constant).

The app also supports debug logging via Hardware Serial protocol.


!!!Currently is heavily WIP!!!

# Configuration


## Hardware configuration
The most important part is to select appropriate pins for communication with the device:
```
int tx_pin = D1;
int rx_pin = D2;
```

It is also important to bind right definitions of serials to the reader:
```
                                |- Serial port used to communicate with the device
                                |
                                |           |- Debug output device
                                |           |
CoReader coReader = CoReader(InputSerial, Serial);
```

## Software configuration
WiFi settings and ThingSpeak configuration (Channel ID and API key) need to be updated upon configuration.
After first run app creates `AutoConnectAP` WiFi network. After connecting by some mobile phone it should redirect to configuration screen.