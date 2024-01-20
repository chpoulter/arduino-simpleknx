/*
 *    SimpleKnxTest.cpp
 *
 *    Written by Christian Poulter.
 *
 *    Copyright (C) 2023 Christian Poulter <devel(at)poulter.de>
 *    All rights reserved. This file is now part of the Ardunio SimpleKnx Library.
 *
 *    The Ardunio SimpleKnx Library is free software: you can redistribute
 *    it and/or modify it under the terms of the GNU General Public License as
 *    published by the Free Software Foundation, either version 3 of the License,
 *    or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "SimpleKnxTest.h"

#define DEVICE_ADDRESS P_ADDR(1, 1, 12)
#define KNX_SERIAL Serial
#define TEST_LED LED_BUILTIN

// I am not a c++ expert, if there is a more elegant way in doing
// this, please let me know.
word SimpleKnx_::_groupAddressList[] = {
    G_ADDR(2,7,1),
    G_ADDR(2,7,2),
    G_ADDR(2,7,3),
    G_ADDR(2,7,4),
    G_ADDR(2,7,5),
    G_ADDR(2,7,6),
    G_ADDR(2,7,7),
    G_ADDR(2,7,8),
    G_ADDR(2,7,9)
};
const byte SimpleKnx_::_groupAddressListSize = sizeof (_groupAddressList) / sizeof (word);

// program
unsigned long blinkDelay;
unsigned long lastmillis;
bool laststate;

// ################################################
// ### SETUP
// ################################################
void setup() {
    DEBUG0_PRINTLN(F("INIT"));

    pinMode(TEST_LED, OUTPUT);
    digitalWrite(TEST_LED, LOW);

    blinkDelay = 500;
    lastmillis = millis();
    laststate = false;

    SimpleKnx.init(KNX_SERIAL, DEVICE_ADDRESS);

    DEBUG0_PRINTLN(F("INIT DONE"));
}

// ################################################
// ### LOOP
// ################################################
void loop() {
    SimpleKnx.task();

    unsigned long currentmillis = millis();

    if (currentmillis - lastmillis >= blinkDelay) {
        laststate = !laststate;
        lastmillis = currentmillis;

        digitalWrite(TEST_LED, laststate ? HIGH : LOW);
    }
}

// ################################################
// ### KNX EVENT CALLBACK
// ################################################
void telegramEventCallback(KnxTelegram telegram) {

    // only interessted in group events
    if (!telegram.isMulticast()) {
        return;
    }

    DEBUG0_PRINTLN(F("---\nTelegram: %d\n---"), telegram.getCommand());

    switch (telegram.getCommand()) {
        case KNX_COMMAND_VALUE_READ:
            telegramEventRead(telegram);
            break;

        case KNX_COMMAND_VALUE_RESPONSE:
        case KNX_COMMAND_VALUE_WRITE:
            telegramEventWrite(telegram);
            break;

        default:
            break;
    }
}

void telegramEventRead(KnxTelegram telegram) {

    if (telegram.getTargetAddress() == G_ADDR(2,7,1)) {
        bool value = true;
        SimpleKnx.groupWriteBool(true, G_ADDR(2,7,1), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,2)) {
        byte value = B00000011;
        SimpleKnx.groupWrite2BitIntValue(true, G_ADDR(2,7,2), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,3)) {
        byte value = B00001001;
        SimpleKnx.groupWrite4BitIntValue(true, G_ADDR(2,7,3), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,4)) {
        byte value = 123;
        SimpleKnx.groupWrite1ByteIntValue(true, G_ADDR(2,7,4), value);

        value = -123;
        SimpleKnx.groupWrite1ByteIntValue(true, G_ADDR(2,7,4), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,5)) {
        int value = 1234;
        SimpleKnx.groupWrite2ByteIntValue(true, G_ADDR(2,7,5), value);

        value = -1234;
        SimpleKnx.groupWrite2ByteIntValue(true, G_ADDR(2,7,5), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,6)) {
        float value = 123.56;
        SimpleKnx.groupWrite2ByteFloatValue(true, G_ADDR(2,7,6), value);

        value = -123.56;
        SimpleKnx.groupWrite2ByteFloatValue(true, G_ADDR(2,7,6), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,7)) {
        float value = 123456.78;
        SimpleKnx.groupWrite4ByteFloatValue(true, G_ADDR(2,7,7), value);

        value = -123456.78;
        SimpleKnx.groupWrite4ByteFloatValue(true, G_ADDR(2,7,7), value);
    }
}

void telegramEventWrite(KnxTelegram telegram) {

    if (telegram.getTargetAddress() == G_ADDR(2,7,1)) {
        bool value = telegram.getBool();

        DEBUG0_PRINTLN(F("Value: %d"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,2)) {
        byte value = telegram.get2BitIntValue();

        DEBUG0_PRINTLN(F("Value: %d"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,3)) {
        byte value = telegram.get4BitIntValue();

        DEBUG0_PRINTLN(F("Value: %d"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,4)) {
        byte value = telegram.get1ByteIntValue();

        DEBUG0_PRINTLN(F("Value: %d"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,5)) {
        int value = telegram.get2ByteIntValue();

        DEBUG0_PRINTLN(F("Value: %d"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,6)) {
        float value = telegram.get2ByteFloatValue();

        DEBUG0_PRINTLN(F("Value: %f"), value);
    }

    if (telegram.getTargetAddress() == G_ADDR(2,7,7)) {
        float value = telegram.get4ByteFloatValue();

        DEBUG0_PRINTLN(F("Value: %f"), value);
    }

}
