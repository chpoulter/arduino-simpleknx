/*
 *    KnxTelegram.cpp
 *
 *    Written by Alexander Christian.
 *    Heavily changed and adopted by Christian Poulter.
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
 *    This file incorporates work covered by the following copyright and
 *    permission notice:
 *
 *    Copyright (C) 2016 Alexander Christian <info(at)root1.de>
 *    All rights reserved. This file is part of KONNEKTING Device Library.
 *
 *    The KONNEKTING Device Library is free software: you can redistribute
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

#include "KnxTelegram.h"
#include "DebugUtil.h"

KnxTelegram::KnxTelegram() {
    clearTelegram(); 
};

// clear telegram with default values :
// std FF, no repeat, normal prio, empty payload
// multicast, routing counter = 6, payload length = 1
void KnxTelegram::clearTelegram(void) {
    memset(_telegram, 0, KNX_TELEGRAM_MAX_SIZE);
  
    _controlField = CONTROL_FIELD_DEFAULT_VALUE; 
    _routing= ROUTING_FIELD_DEFAULT_VALUE;
}

byte KnxTelegram::calculateChecksum(void) const {

    byte xorSum = 0;
    byte indexChecksum = KNX_TELEGRAM_HEADER_SIZE + getPayloadLength() + 1;
    for (byte i = 0; i < indexChecksum ; i++) {
        xorSum ^= _telegram[i]; // XOR Sum of all the databytes
    }
  
    return byte(~xorSum); // Checksum equals 1's complement of databytes XOR sum
}

void KnxTelegram::updateChecksum(void) {
	byte indexChecksum = KNX_TELEGRAM_HEADER_SIZE + getPayloadLength() + 1;
    _telegram[indexChecksum] = calculateChecksum();
}

void KnxTelegram::copy(KnxTelegram& dest) const {
    byte length = getTelegramLength();

    memcpy(dest._telegram, _telegram, length);
}

void KnxTelegram::copyHeader(KnxTelegram& dest) const {
    memcpy(dest._telegram, _telegram, KNX_TELEGRAM_HEADER_SIZE);
}

KnxTelegramValidity KnxTelegram::getValidity(void) const {
    if ((_controlField & CONTROL_FIELD_PATTERN_MASK) != CONTROL_FIELD_VALID_PATTERN)
        return KNX_TELEGRAM_INVALID_CONTROL_FIELD;

    if ((_controlField & CONTROL_FIELD_FRAME_FORMAT_MASK) != CONTROL_FIELD_STANDARD_FRAME_FORMAT)
        return KNX_TELEGRAM_UNSUPPORTED_FRAME_FORMAT;

    if (!getPayloadLength())
        return KNX_TELEGRAM_INCORRECT_PAYLOAD_LENGTH ;

    if ((_commandH & COMMAND_FIELD_PATTERN_MASK) != COMMAND_FIELD_VALID_PATTERN)
        return KNX_TELEGRAM_INVALID_COMMAND_FIELD;

    if ( getChecksum() != calculateChecksum())
        return KNX_TELEGRAM_INCORRECT_CHECKSUM ;
  
    byte cmd=getCommand();
    if  (    (cmd!=KNX_COMMAND_VALUE_READ)  && (cmd!=KNX_COMMAND_VALUE_RESPONSE)
          && (cmd!=KNX_COMMAND_VALUE_WRITE) && (cmd!=KNX_COMMAND_MEMORY_WRITE))
        return KNX_TELEGRAM_UNKNOWN_COMMAND;
        
    return  KNX_TELEGRAM_VALID;
};

void KnxTelegram::setPayload(const byte data[], byte length) {
    
    if (length == 0) {
        _commandL &= ~COMMAND_FIELD_LOW_DATA_MASK;
        _commandL |= data[0] & COMMAND_FIELD_LOW_DATA_MASK;
        
    } else {
        setPayloadLength(length + 1);
        
        length = min(length, KNX_TELEGRAM_PAYLOAD_MAX_SIZE-2);
        for(byte i=0; i < length; i++) _payloadChecksum[i] = data[i];
    }
}

// --------------- DPT functions ---------------
bool KnxTelegram::getBool() {
    if (getPayloadLength() != 1) { return 0; }

    return _commandL & B00000001;
}

byte KnxTelegram::get2BitIntValue() {
    if (getPayloadLength() != 1) { return 0; }

    return _commandL & B00000011;
}

byte KnxTelegram::get4BitIntValue() {
    if (getPayloadLength() != 1) { return 0; }

    return _commandL & B00001111;
}

byte KnxTelegram::get1ByteIntValue() {
    if (getPayloadLength() != 2) { return 0; }

    return _payloadChecksum[0];
}

int KnxTelegram::get2ByteIntValue() {
    if (getPayloadLength() != 3) { return 0; }
  
    return int(_payloadChecksum[0] << 8) + int(_payloadChecksum[1]);
}

float KnxTelegram::get2ByteFloatValue() {
    if (getPayloadLength() != 3) { return 0; }

    int signMultiplier = (_payloadChecksum[0] & 0x80) ? -1 : 1;
    word absoluteMantissa = _payloadChecksum[1] + ((_payloadChecksum[0] & 0x07) << 8);
    if (signMultiplier == -1) {  // Calculate absolute mantissa value in case of negative mantissa
        // Abs = 2's complement + 1
        absoluteMantissa = ((~absoluteMantissa) & 0x07FF) + 1;
    }
    byte exponent = (_payloadChecksum[0] & 0x78) >> 3;
    
    return (0.01 * ((long)absoluteMantissa << exponent) * signMultiplier);
}

float KnxTelegram::get4ByteFloatValue() {
    if (getPayloadLength() != 5) {
        return 0;
    }
  
    byte data[4];
    data[0] = _payloadChecksum[3];
    data[1] = _payloadChecksum[2];
    data[2] = _payloadChecksum[1];
    data[3] = _payloadChecksum[0];
    float *value = (float*)(void*) & (data[0]);
    float  result = *value;

    return result;
}
