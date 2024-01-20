# SimpleKnx Library

The SimpleKnx Library allows access to the KNX bus from an Ardunio board.

## Connecting the hardware  

Beside of an ardunio I use two components:
1. Siemens 5WG1 117-2AB12
2. ADUM2101

```
Siemens 5WG1 117-2AB12
 _____________
/  a ooooo    \
|  b ooooo    |
|    12345    |
|             |
|     KNX     |
\_____________/
     -| |+

Pegelwandler ADUM1201

V1 o .-----. o V2
A0 o |     | o A1
B1 o |     | o B0
G1 o `-----´ o G2   

Siem 5WG1     ADUM1201
a1   +5V   ->   V2
a2         ->   A1
a4         ->   B0
a5   GND   ->   G2

a3 and all b not used

ADUM2101     Arduino
   V1    ->    VCC
   A0    ->    RXD
   B1    ->    TXD
   G1    ->    GND
```

## Example

```

#include "Arduino.h"
#include <DebugUtil.h>
#include <SimpleKnx.h>

#define DEVICE_ADDRESS P_ADDR(1, 1, 12)
#define KNX_SERIAL Serial

word SimpleKnx_::_groupAddressList[] = {
    G_ADDR(1,1,1), ...
};
const byte SimpleKnx_::_groupAddressListSize = sizeof (_groupAddressList) / sizeof (word);

void setup() {
    SimpleKnx.init(KNX_SERIAL, DEVICE_ADDRESS);
}

void loop() {

   // make sure to call this often enough
   SimpleKnx.task();
}

// this callback will be executed for every telegram matching
// the _groupAddressList specified at the top.
void telegramEventCallback(KnxTelegram telegram) {
   ... do something with the telegram ...
   
    byte command = telegram.getCommand();
   
    bool value = telegram.getBool();
    byte value = telegram.get2BitIntValue();
    byte value = telegram.get4BitIntValue();
    byte value = telegram.get1ByteIntValue();
    int value = telegram.get2ByteIntValue();
    float value = telegram.get2ByteFloatValue();
    float value = telegram.get4ByteFloatValue();
}

void writeTelegram() {
    ... send a telegram ...
    
    SimpleKnx.groupWriteBool(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite2BitIntValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite4BitIntValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite1ByteIntValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite2ByteIntValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite4ByteIntValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite2ByteFloatValue(true, G_ADDR(1,1,1), value);
    SimpleKnx.groupWrite4ByteFloatValue(true, G_ADDR(1,1,1), value);
}
```

For a full example the SimpleKnxTest in the example folder.

## Debugging

My arduinos only have one serial port, so I used [SoftwareSerial](http://www.arduino.cc/en/Reference/SoftwareSerial)
to emulate a second serial port on pins 8 and 9. The debug code is only included when using -DDEBUGGING_0 .. -DDEBUGGING_5
compile options to save around 4 kb space. I could not find a way to change compiler options with [ArdunioIDE](https://www.arduino.cc/en/software), so
I switched to [Eclipse Sloeber](https://eclipse.baeyens.it), which seems to be more advanced.

## Licence
This library is released under the GNU GENERAL PUBLIC LICENSE Version 3 license, for more information, check the LICENSE file.

Parts of this library are based on the [KonnektingDeviceLibrary](https://gitlab.com/konnekting/KonnektingDeviceLibrary). 