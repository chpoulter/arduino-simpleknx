/*
 *    SimpleKnx.h
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
 *    This file includes work covered by the following copyright and
 *    permission notice:
 *
 *    This file is part of KONNEKTING Device Library.
 *
 *    The KONNEKTING Device Library is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
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

#ifndef SimpleKnx_h
#define SimpleKnx_h

#include <Arduino.h>
#include <avr/wdt.h>

#include "RingBuff.h"
#include "KnxTpUart.h"

#define ACTIONS_QUEUE_SIZE 16
#define KNX_RXTASK_INTERVAL 400
#define KNX_TXTASK_INTERVAL 800

// Macro functions for conversion of physical and group addresses
inline word P_ADDR(byte area, byte line, byte busdevice) { return (word) ( ((area&0xF)<<12) + ((line&0xF)<<8) + busdevice ); }
inline word G_ADDR(byte maingrp, byte midgrp, byte subgrp) { return (word) ( ((maingrp&0x1F)<<11) + ((midgrp&0x7)<<8) + subgrp ); }

// Values returned by the KnxDevice functions
enum KnxDeviceStatus {
    KNX_DEVICE_OK = 0,
    KNX_DEVICE_INIT_ERROR = 2
};

class SimpleKnx_ {

    public:
        static word _groupAddressList[];
        static const byte _groupAddressListSize;

        static SimpleKnx_ &getInstance();
        SimpleKnx_(const SimpleKnx_ &) = delete;
        SimpleKnx_ &operator=(const SimpleKnx_ &) = delete;
        
        void init(HardwareSerial &serial, word deviceAddress);
        void task(void);
        
        void groupWriteBool(bool answer, word groupAddress, bool value);
        void groupWrite2BitIntValue(bool answer, word groupAddress, byte value);
        void groupWrite4BitIntValue(bool answer, word groupAddress, byte value);
        void groupWrite1ByteIntValue(bool answer, word groupAddress, byte value);
        void groupWrite2ByteIntValue(bool answer, word groupAddress, int value);
        void groupWrite4ByteIntValue(bool answer, word groupAddress, long value);
        void groupWrite2ByteFloatValue(bool answer, word groupAddress, float value);
        void groupWrite4ByteFloatValue(bool answer, word groupAddress, float value);
 
    private:
        SimpleKnx_();
            
        word _deviceAddress;
        word _lastRXTimeMicros;
        word _lastTXTimeMicros;
        KnxTpUart *_tpuart;
        KnxTelegram *_rxTelegram;
        KnxTelegram _txTelegram;        
        RingBuff<KnxTelegram, ACTIONS_QUEUE_SIZE> _txActionList;

        void reboot();
        KnxDeviceStatus begin(HardwareSerial& serial);
        void end();

        void appendTelegram(bool answer, word groupAddress, byte data[], byte length);
        static void getTpUartEvents(KnxTpUartEvent event);
};

extern void telegramEventCallback(KnxTelegram telegram);
extern SimpleKnx_ &SimpleKnx;

#endif
