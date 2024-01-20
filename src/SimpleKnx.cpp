/*
 *    SimpleKnx.cpp
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

#include <Arduino.h>
#include "SimpleKnx.h"
#include "DebugUtil.h"
#include "KnxTools.h"

SimpleKnx_ &SimpleKnx = SimpleKnx.getInstance();

SimpleKnx_::SimpleKnx_() {
    _tpuart = NULL;
}

void SimpleKnx_::init(HardwareSerial &serial, word deviceAddress) {
    _deviceAddress = deviceAddress;
    
    KnxDeviceStatus status = begin(serial);

    if (status != KNX_DEVICE_OK) {
        delay(500);
        reboot();
    }
}

void SimpleKnx_::reboot() {
    end();

    // to overcome WDT infinite reboot-loop issue
    // see: https://github.com/arduino/Arduino/issues/4492
    delay(500);
    asm volatile("jmp 0");    
}

KnxDeviceStatus SimpleKnx_::begin(HardwareSerial& serial) {
    delete _tpuart;
    _tpuart = new KnxTpUart(serial, _deviceAddress, _groupAddressList, _groupAddressListSize);
    
    _rxTelegram = &_tpuart->getReceivedTelegram();
    if (_tpuart->reset() != KNX_TPUART_OK) {
        delete (_tpuart);
        
        _tpuart = NULL;
        _rxTelegram = NULL;
        
        return KNX_DEVICE_INIT_ERROR;
    }

    _tpuart->setEvtCallback(&SimpleKnx_::getTpUartEvents);
    _tpuart->init();

    _lastRXTimeMicros = micros();
    _lastTXTimeMicros = _lastRXTimeMicros;

    return KNX_DEVICE_OK;
}

// Stop the KNX Device
void SimpleKnx_::end() {
    while ( _txActionList.getItemCount() > 0 ) {
        task();
    }
    
    _rxTelegram = NULL;

    delete (_tpuart);
    _tpuart = NULL;
}

SimpleKnx_ &SimpleKnx_::getInstance() {
  static SimpleKnx_ instance;
  
  return instance;
}

void SimpleKnx_::getTpUartEvents(KnxTpUartEvent event) {

    switch (event) {
      
        // Manage RECEIVED MESSAGES
        case TPUART_EVENT_RECEIVED_KNX_TELEGRAM: {
            KnxTelegram telegram = SimpleKnx._tpuart->getReceivedTelegram();
            telegramEventCallback(telegram);

        } break;
        
        // Manage RESET events
        case TPUART_EVENT_RESET: {
            
            // wait for successfull reset
            while (SimpleKnx._tpuart->reset() == KNX_TPUART_ERROR) {}
                
            SimpleKnx._tpuart->init();
        } break;
        
        // noop
        default: {}
        
    }
}

void SimpleKnx_::task(void) {

    do {
        word nowTimeMicros = micros();
        
        DEBUG5_PRINTLN(F("SimpleKnx task %lu"), nowTimeMicros);
        
        // STEP 1: Get new received KNX messages from the TPUART
        if (TimeDeltaWord(nowTimeMicros, _lastRXTimeMicros) > KNX_RXTASK_INTERVAL) {
            _lastRXTimeMicros = nowTimeMicros;
            _tpuart->rxTask();

            while ( _tpuart->isRxActive() ) {
                _tpuart->rxTask();
            }
        }

        // STEP 2: Send KNX messages following TX actions
        if (_tpuart->isFreeToSend() && _txActionList.pop(_txTelegram)) {
            _tpuart->sendTelegram(_txTelegram);
        }

        // STEP 3: LET THE TP-UART TRANSMIT KNX MESSAGES
        nowTimeMicros = micros();
        if (TimeDeltaWord(nowTimeMicros, _lastTXTimeMicros) > KNX_TXTASK_INTERVAL) {
            _lastTXTimeMicros = nowTimeMicros;
            _tpuart->txTask();
        }
        
    } while (_tpuart->isActive());
}

void SimpleKnx_::appendTelegram(bool answer, word groupAddress, byte data[], byte length) {
    KnxTelegram txTelegram;
    txTelegram.setTargetAddress(groupAddress);
    txTelegram.setCommand(answer ? KNX_COMMAND_VALUE_RESPONSE : KNX_COMMAND_VALUE_WRITE);
    txTelegram.setPayload(data, length);

    DEBUG2_PRINTLN(F("appendTelegram ga=0x%04x length=%d data=0x%02x"), txTelegram.getTargetAddress(), length, data[0]);

    _txActionList.append(txTelegram);
}

void SimpleKnx_::groupWriteBool(bool answer, word groupAddress, bool value) {
    byte data[] = { byte(value ? B00000001 : B00000000) };
    appendTelegram(answer, groupAddress, data, 0);
}

void SimpleKnx_::groupWrite2BitIntValue(bool answer, word groupAddress, byte value) {
    byte data[] = { byte(value & B00000011) };
    appendTelegram(answer, groupAddress, data, 0);
}

void SimpleKnx_::groupWrite4BitIntValue(bool answer, word groupAddress, byte value) {
    byte data[] = { byte(value & B00001111) };
    appendTelegram(answer, groupAddress, data, 0);
}

void SimpleKnx_::groupWrite1ByteIntValue(bool answer, word groupAddress, byte value) {
    byte data[] = { value };
    appendTelegram(answer, groupAddress, data, 1);
}

void SimpleKnx_::groupWrite2ByteIntValue(bool answer, word groupAddress, int value) {
    byte data[] = { byte(value >> 8), byte(value & 0xFF) };
    appendTelegram(answer, groupAddress, data, 2);
}


void SimpleKnx_::groupWrite4ByteIntValue(bool answer, word groupAddress, long value) {
    byte data[] = { byte(value >> 24), byte(value >> 16), byte(value >> 8), byte(value & 0xFF) };
    appendTelegram(answer, groupAddress, data, 4);
}

void SimpleKnx_::groupWrite2ByteFloatValue(bool answer, word groupAddress, float value) {
    byte data[2];
    
    long longValuex100 = (long)(100.0 * value);
    bool negativeSign = (longValuex100 & 0x80000000) ? true : false;
    byte exponent = 0;
    byte round = 0;

    if (negativeSign) {
        while (longValuex100 < (long)(-2048)) {
            exponent++;
            round = (byte)(longValuex100)&1;
            longValuex100 >>= 1;
            longValuex100 |= 0x80000000;
        }
    } else {
        while (longValuex100 > (long)(2047)) {
            exponent++;
            round = (byte)(longValuex100)&1;
            longValuex100 >>= 1;
        }
    }
    
    if (round) longValuex100++;
    data[1] = (byte)longValuex100;
    data[0] = (byte)(longValuex100 >> 8) & 0x7;
    data[0] += exponent << 3;    
    if (negativeSign) data[0] += 0x80;

    appendTelegram(answer, groupAddress, data, 2);
}

void SimpleKnx_::groupWrite4ByteFloatValue(bool answer, word groupAddress, float value) {
    byte data[4];
    float *f = (float*)(void*) & (data[0]);
    *f = value;
    
    appendTelegram(answer, groupAddress, data, 4);
}
