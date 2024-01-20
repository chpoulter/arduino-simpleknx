/*
 *    KnxTelegram.h
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

#ifndef KNXTELEGRAM_H
#define KNXTELEGRAM_H

#include <Arduino.h>

// ---------- Knx Telegram description (visit "www.knx.org" for more info) -----------
// => Length : 9 bytes min. to 23 bytes max.
//
// => Structure :
//      -Header (6 bytes):
//        Byte 0 | Control Field
//        Byte 1 | Source Address High byte
//        Byte 2 | Source Address Low byte
//        Byte 3 | Destination Address High byte
//        Byte 4 | Destination Address Low byte
//        Byte 5 | Routing field
//      -Payload (from 2 up to 16 bytes):
//        Byte 6 | Commmand field High
//        Byte 7 | Command field Low + 1st payload data (6bits)
//        Byte 8 up to 21 | payload bytes (optional)
//      -Checksum (1 byte)
//
// => Fields details :
//      -Control Field : "FFR1 PP00" format with
//         FF = Frame Format (10 = Std Length L_DATA service, 00 = extended L_DATA service, 11 = L_POLLDATA service)
//          R = Repeatflag (1 = not repeated, 0 = repeated)
//         PP = Priority (00 = system, 10 = alarm, 01 = high, 11 = normal)
//      -Routing Field  : "TCCC LLLL" format with
//          T = Target Addr type (1 = group address/muticast, 0 = individual address/unicast)
//        CCC = Counter
//       LLLL = Payload Length (1-15)
//      -Command Field : "00XX XXCC CCDD DDDD" format with
//         XX = Not used
//         CC = command (0000 = Value Read, 0001 = Value Response, 0010 = Value Write, 1010 = Memory Write)
//         DD = Payload Data (1st payload byte)
//
// => Transmit timings :
//     -Tbit = 104us, Tbyte=1,35ms (13 bits per character)
//     -from 20ms for 1 byte payload telegram (Bus temporisation + Telegram transmit + ACK)
//     -up to 40ms for 15 bytes payload (Bus temporisation + Telegram transmit + ACK)
//

// Define for lengths & offsets
#define KNX_TELEGRAM_HEADER_SIZE        6
#define KNX_TELEGRAM_PAYLOAD_MAX_SIZE  16
#define KNX_TELEGRAM_MIN_SIZE           9
#define KNX_TELEGRAM_MAX_SIZE          23
#define KNX_TELEGRAM_LENGTH_OFFSET      8 // Offset between payload length and telegram length

enum KnxPriority {
  KNX_PRIORITY_SYSTEM_VALUE  = 0b00000000,
  KNX_PRIORITY_HIGH_VALUE    = 0b00000100,
  KNX_PRIORITY_ALARM_VALUE   = 0b00001000,
  KNX_PRIORITY_NORMAL_VALUE  = 0b00001100
};

enum KnxCommand {
  KNX_COMMAND_VALUE_READ     = 0b00000000,
  KNX_COMMAND_VALUE_RESPONSE = 0b00000001,
  KNX_COMMAND_VALUE_WRITE    = 0b00000010,
  KNX_COMMAND_MEMORY_WRITE   = 0b00001010
};

//--- CONTROL FIELD values & masks ---
#define CONTROL_FIELD_DEFAULT_VALUE         0b10111100 // Standard FF; No Repeat; Normal Priority
#define CONTROL_FIELD_FRAME_FORMAT_MASK     0b11000000
#define CONTROL_FIELD_STANDARD_FRAME_FORMAT 0b10000000
#define CONTROL_FIELD_REPEATED_MASK         0b00100000
#define CONTROL_FIELD_SET_REPEATED(x)       (x&=0b11011111)
#define CONTROL_FIELD_PRIORITY_MASK         0b00001100
#define CONTROL_FIELD_PATTERN_MASK          0b00010011
#define CONTROL_FIELD_VALID_PATTERN         0b00010000

// --- ROUTING FIELD values & masks ---
#define ROUTING_FIELD_DEFAULT_VALUE            0b11100001 // Multicast(Target Group @), Routing Counter = 6, Length = 1
#define ROUTING_FIELD_TARGET_ADDRESS_TYPE_MASK 0b10000000
#define ROUTING_FIELD_COUNTER_MASK             0b01110000 
#define ROUTING_FIELD_PAYLOAD_LENGTH_MASK      0b00001111

// --- COMMAND FIELD values & masks ---
#define COMMAND_FIELD_HIGH_COMMAND_MASK 0x03 
#define COMMAND_FIELD_LOW_COMMAND_MASK  0xC0 // 2 first bytes on _commandL
#define COMMAND_FIELD_LOW_DATA_MASK     0x3F // 6 last bytes are data
#define COMMAND_FIELD_PATTERN_MASK      0b11000000
#define COMMAND_FIELD_VALID_PATTERN     0b00000000

enum KnxTelegramValidity {
    KNX_TELEGRAM_VALID = 0 ,
    KNX_TELEGRAM_INVALID_CONTROL_FIELD,
    KNX_TELEGRAM_UNSUPPORTED_FRAME_FORMAT,
    KNX_TELEGRAM_INCORRECT_PAYLOAD_LENGTH,
    KNX_TELEGRAM_INVALID_COMMAND_FIELD,
    KNX_TELEGRAM_UNKNOWN_COMMAND,
    KNX_TELEGRAM_INCORRECT_CHECKSUM
};

class KnxTelegram {
  
    union {
        byte _telegram[KNX_TELEGRAM_MAX_SIZE]; // byte 0 to 22
        struct {
            byte _controlField; // byte 0
            byte _sourceAddrH;  // byte 1
            byte _sourceAddrL;  // byte 2
            byte _targetAddrH;  // byte 3
            byte _targetAddrL;  // byte 4
            byte _routing;      // byte 5
            byte _commandH;     // byte 6
            byte _commandL;     // byte 7
            byte _payloadChecksum[KNX_TELEGRAM_PAYLOAD_MAX_SIZE-1]; // byte 8 to 22
        };
    };

  public:
    KnxTelegram();
    void copy(KnxTelegram& dest) const;
    void copyHeader(KnxTelegram& dest) const;
    
    void setPriority(KnxPriority priority);
    KnxPriority getPriority(void) const;

    void setRepeated(void);
    boolean isRepeated(void) const;

    void setSourceAddress(word addr);
    word getSourceAddress(void) const;
    
    void setTargetAddress(word addr);
    word getTargetAddress(void) const;

    void setMulticast(boolean);
    boolean isMulticast(void) const;

    void setRoutingCounter(byte counter);
    byte getRoutingCounter(void) const;

    void setPayloadLength(byte length);
    byte getPayloadLength(void) const;

    byte getTelegramLength(void) const;

    void setCommand(KnxCommand cmd);
    KnxCommand getCommand(void) const;

    byte getChecksum(void) const;
    boolean isChecksumCorrect(void) const;

    // raw data getter setter
    void setPayload(const byte data[], byte length);
    byte getRawByte(byte byteIndex) const;
    void setRawByte(byte data, byte byteIndex);

    // checksum
    void clearTelegram(void); // (re)set telegram with default values
    byte calculateChecksum(void) const;
    void updateChecksum(void);
    KnxTelegramValidity getValidity(void) const;

    // get DPT
    bool getBool();
    byte get2BitIntValue();
    byte get4BitIntValue();
    byte get1ByteIntValue();
    int get2ByteIntValue();
    float get2ByteFloatValue();
    float get4ByteFloatValue();
    
};

// --------------- Definition of the INLINED functions : -----------------
inline void KnxTelegram::setPriority(KnxPriority priority) { 
    _controlField &= ~CONTROL_FIELD_PRIORITY_MASK;
    _controlField |= priority & CONTROL_FIELD_PRIORITY_MASK;
}
    
inline KnxPriority KnxTelegram::getPriority(void) const {
    return (KnxPriority)(_controlField & CONTROL_FIELD_PRIORITY_MASK);
}

inline void KnxTelegram::setRepeated(void ) {
    CONTROL_FIELD_SET_REPEATED(_controlField);
};
    
inline boolean KnxTelegram::isRepeated(void) const {
    return !(_controlField & CONTROL_FIELD_REPEATED_MASK );
}

// WARNING : works with little endianness only
// The adresses within KNX telegram are big endian
inline void KnxTelegram::setSourceAddress(word addr) { 
    _sourceAddrL = (byte) addr;
    _sourceAddrH = byte(addr>>8);
}

// WARNING : works with little endianness only
// The adresses within KNX telegram are big endian
inline word KnxTelegram::getSourceAddress(void) const {
    return _sourceAddrL + (_sourceAddrH<<8);
}

// WARNING : works with little endianness only
// The adresses within KNX telegram are big endian
inline void KnxTelegram::setTargetAddress(word addr) { 
    _targetAddrL = (byte) addr;
    _targetAddrH = byte(addr>>8);
}

// WARNING : endianess sensitive!! Code below is for LITTLE ENDIAN chip
// The KNX telegram uses BIG ENDIANNESS (Hight byte placed before Low Byte)
inline word KnxTelegram::getTargetAddress(void) const {
    return _targetAddrL + (_targetAddrH<<8);
}

inline boolean KnxTelegram::isMulticast(void) const {
    return (_routing & ROUTING_FIELD_TARGET_ADDRESS_TYPE_MASK);
}

inline void KnxTelegram::setMulticast(boolean mode) {
    if (mode) {
        _routing |= ROUTING_FIELD_TARGET_ADDRESS_TYPE_MASK;
    } else {
        _routing &= ~ROUTING_FIELD_TARGET_ADDRESS_TYPE_MASK;
    }
}
 
inline void KnxTelegram::setRoutingCounter(byte counter) {
    counter <<= 4;
    _routing &= ~ROUTING_FIELD_COUNTER_MASK;
    _routing |= (counter & ROUTING_FIELD_COUNTER_MASK);
}

inline byte KnxTelegram::getRoutingCounter(void) const {
    return ((_routing & ROUTING_FIELD_COUNTER_MASK)>>4);
}

inline void KnxTelegram::setPayloadLength(byte length) {
    _routing &= ~ROUTING_FIELD_PAYLOAD_LENGTH_MASK;
    _routing |= length & ROUTING_FIELD_PAYLOAD_LENGTH_MASK;
}

inline byte KnxTelegram::getPayloadLength(void) const {
    return (_routing & ROUTING_FIELD_PAYLOAD_LENGTH_MASK);
}

inline byte KnxTelegram::getTelegramLength(void) const {
    return (KNX_TELEGRAM_LENGTH_OFFSET + getPayloadLength());
}

inline void KnxTelegram::setCommand(KnxCommand cmd) {
    _commandH &= ~COMMAND_FIELD_HIGH_COMMAND_MASK;
    _commandH |= (cmd >> 2);
    
    _commandL &= ~COMMAND_FIELD_LOW_COMMAND_MASK;
    _commandL |= (cmd << 6);
}

inline KnxCommand KnxTelegram::getCommand(void) const {
    return (KnxCommand)(((_commandL & COMMAND_FIELD_LOW_COMMAND_MASK)>>6) + 
                        ((_commandH & COMMAND_FIELD_HIGH_COMMAND_MASK)<<2));
};

inline byte KnxTelegram::getRawByte(byte byteIndex) const {
    return _telegram[byteIndex];
}

inline void KnxTelegram::setRawByte(byte data, byte byteIndex) {
    _telegram[byteIndex] = data;
}

inline byte KnxTelegram::getChecksum(void) const {
    return (_payloadChecksum[getPayloadLength() - 1]);
}

inline boolean KnxTelegram::isChecksumCorrect(void) const {
    return (getChecksum()==calculateChecksum());
}

#endif // KNXTELEGRAM_H
