# Description
This simple app is intended to read from solar driver via Serial protocol and then push it to MQQT
The app also supports debug logging via Hardware Serial protocol.

!!!Currently is heavily WIP!!!

# Configuration

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
