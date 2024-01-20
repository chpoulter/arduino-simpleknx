/*
 *    KnxTpuart.cpp
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
 *    Copyright (C) 2016 Alexander Christian <info(at)root1.de>. All rights
 *    reserved. This file is part of KONNEKTING Device Library.
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

#include <avr/pgmspace.h>
#include "KnxTpUart.h"
#include "DebugUtil.h"
#include "KnxTools.h"

// Constructor
KnxTpUart::KnxTpUart(HardwareSerial& serial, word physicalAddr, word groupAddressList[], byte groupAddressListSize):
    _serial(serial),
    _physicalAddr(physicalAddr),
    _groupAddressList(groupAddressList),
    _groupAddressListSize(groupAddressListSize)
{
    DEBUG0_PRINTLN("KnxTpUart")
      
    _rx.state = RX_RESET;
    _rx.readBytes = 0;
    
    _tx.state = TX_RESET;
    _tx.sentTelegram = NULL;
    _tx.bytesRemaining = 0;
    _tx.txByteIndex = 0;
    
    _evtCallbackFct = NULL;
}

// Destructor
KnxTpUart::~KnxTpUart() {
    if ((_rx.state > RX_RESET) || (_tx.state > TX_RESET)) {
        _serial.end();
    }    
}

// Reset the Arduino UART port and the TPUART device
byte KnxTpUart::reset(void) {
    word startTime, nowTime;
    byte attempts = 10;

    DEBUG0_PRINTLN(F("Reset triggered!"));

    // HOT RESET case
    if ((_rx.state > RX_RESET) || (_tx.state > TX_RESET)) {
        DEBUG0_PRINTLN(F("HOT RESET case"));
        // stop the serial communication before restarting it
        _serial.end();
        _rx.state = RX_RESET;
        _tx.state = TX_RESET;
    }
    
    // CONFIGURATION OF THE ARDUINO UART WITH CORRECT FRAME FORMAT (19200, 8 bits, parity even, 1 stop bit)
    _serial.begin(19200, SERIAL_8E1);

    while (attempts--) {  // we send a RESET REQUEST and wait for the reset indication answer

        DEBUG0_PRINTLN(F("Reset attempts: %d"), attempts);

        // the sequence is repeated every sec as long as we do not get the reset indication
        _serial.write(TPUART_RESET_REQ);  // send RESET REQUEST

        for (nowTime = startTime = (word)millis(); TimeDeltaWord(nowTime, startTime) < 1000 /* 1 sec */; nowTime = (word)millis()) {
          
            if (_serial.available() > 0) {
                DEBUG0_PRINTLN("Data available: %d", _serial.available());
                byte data = _serial.read();
                
                if (data == TPUART_RESET_INDICATION) {
                    _rx.state = RX_INIT;
                    _tx.state = TX_INIT;
                    
                    DEBUG0_PRINTLN(F("Reset successful"));
                    
                    return KNX_TPUART_OK;
                    
                } else {
                    DEBUG0_PRINTLN("data not useable: 0x%02x. Expected: 0x%02x", data, TPUART_RESET_INDICATION);
                }
            }
        }
    }
    
    _serial.end();
    
    DEBUG0_PRINTLN(F("Reset failed, no answer from TPUART device"));
    
    return KNX_TPUART_ERROR;
}

// Init
byte KnxTpUart::init(void) {
    if ((_rx.state != RX_INIT) || (_tx.state != TX_INIT)) {
        return KNX_TPUART_ERROR_NOT_INIT_STATE;
    }

    _rx.state = RX_IDLE_WAITING_FOR_CTRL_FIELD;
    _tx.state = TX_IDLE;

    DEBUG0_PRINTLN(F("Init : Normal mode started\n"));

    return KNX_TPUART_OK;
}

byte KnxTpUart::setEvtCallback(EventCallbackFctPtr evtCallbackFct) { 
    if (evtCallbackFct == NULL) return KNX_TPUART_ERROR;
    if ((_rx.state!=RX_INIT) || (_tx.state!=TX_INIT)) return KNX_TPUART_ERROR_NOT_INIT_STATE;

    _evtCallbackFct = evtCallbackFct;
  
    return KNX_TPUART_OK;
}

/*
 * Reception task
 * 
 * DO NOT PUT TOO MUCH DEBUG PRINT CODE HERE! Telegram receiving might break!
 */
void KnxTpUart::rxTask(void) {  
    byte incomingByte;
    unsigned long nowTime;
    
    static byte expectedTelegramLength = 0;
    static KnxTelegram telegram;
    static unsigned long lastByteRxTimeMicrosec;

    nowTime = micros();
    DEBUG5_PRINTLN(F("RxTask: %lu %lu %lu %d"), nowTime, lastByteRxTimeMicrosec, TimeDeltaUnsignedLong(nowTime, lastByteRxTimeMicrosec), _rx.state);
    
    // === STEP 1 : Check EOP in case a Telegram is being received ===
    if (_rx.state >= RX_KNX_TELEGRAM_RECEPTION_STARTED) {
        if (TimeDeltaUnsignedLong(nowTime, lastByteRxTimeMicrosec) > KNX_RX_TIMEOUT ) {
            DEBUG5_PRINTLN(F("EOP REACHED"));
            rxTaskFinished(telegram);
        }
    }

    // === STEP 2 : Get New RX Data ===
    if (_serial.available() > 0) {
        incomingByte = (byte)(_serial.read());        
        lastByteRxTimeMicrosec = micros();

        DEBUG5_PRINTLN(F("RX:  incomingByte=0x%02x, readBytesNb=%d, state=%d"), incomingByte, _rx.readBytes, _rx.state);

        switch (_rx.state) {
            case RX_IDLE_WAITING_FOR_CTRL_FIELD:
                DEBUG5_PRINTLN(F("RX_IDLE_WAITING_FOR_CTRL_FIELD \nincomingByte=0x%02x, readBytes=%d"), incomingByte, _rx.readBytes);

                // CASE OF KNX MESSAGE
                if ((incomingByte & KNX_CONTROL_FIELD_PATTERN_MASK) == KNX_CONTROL_FIELD_VALID_PATTERN) {
                    _rx.state = RX_KNX_TELEGRAM_RECEPTION_STARTED;
                    _rx.readBytes = 1;
                    telegram.setRawByte(incomingByte, 0);
                    
                    DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_STARTED"));
                
                // CASE OF TPUART_DATA_CONFIRM_SUCCESS NOTIFICATION
                } else if (incomingByte == TPUART_DATA_CONFIRM_SUCCESS) {
                    DEBUG5_PRINTLN(F("TPUART_DATA_CONFIRM_SUCCESS"));
                    
                    if (_tx.state == TX_WAITING_ACK) {
                        _tx.state = TX_IDLE;
                        
                    } else {
                        DEBUG5_PRINTLN(F("Rx: unexpected TPUART_DATA_CONFIRM_SUCCESS received!"));
                    }
                
                // CASE OF TPUART_RESET NOTIFICATION
                } else if (incomingByte == TPUART_RESET_INDICATION) {
                    _tx.state = TX_STOPPED;
                    _rx.state = RX_STOPPED;
                    
                    // Notify RESET
                    _evtCallbackFct(TPUART_EVENT_RESET);
                    
                    DEBUG5_PRINTLN(F("Rx: Reset Indication Received"));
                    
                    return;
                
                // CASE OF STATE_INDICATION RESPONSE
                } else if ((incomingByte & TPUART_STATE_INDICATION_MASK) == TPUART_STATE_INDICATION) {
                    DEBUG5_PRINTLN(F("Rx: State Indication Received"));
                
                // CASE OF TPUART_DATA_CONFIRM_FAILED NOTIFICATION
                } else if (incomingByte == TPUART_DATA_CONFIRM_FAILED) {
                    DEBUG5_PRINTLN(F("TPUART_DATA_CONFIRM_FAILED"));
                    
                    // NACK following Telegram transmission
                    if (_tx.state == TX_WAITING_ACK) {
                        _tx.state = TX_IDLE;
                        
                    } else {
                        DEBUG5_PRINTLN(F("Rx: unexpected TPUART_DATA_CONFIRM_FAILED received!"));
                    }
                
                // UNKNOWN CONTROL FIELD RECEIVED
                } else if (incomingByte) {
                    DEBUG5_PRINTLN(F("Rx: Unknown Control Field received: byte=0x%02x"), incomingByte);
                }
                
                // else ignore "0" value sent on Reset by TPUART prior to TPUART_RESET_INDICATION
                break;

            case RX_KNX_TELEGRAM_RECEPTION_STARTED:
                DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_STARTED incomingByte=0x%02x, readBytesNb=%d"), incomingByte, _rx.readBytes);
                
                telegram.setRawByte(incomingByte, _rx.readBytes);
                _rx.readBytes++;

                // here the control, source and target address byte have been read
                if (_rx.readBytes == 6) { 
                    expectedTelegramLength = (incomingByte & KNX_PAYLOAD_LENGTH_MASK) + 7;

                    if ((telegram.getSourceAddress() != _physicalAddr) && isAddressAssigned(telegram.getTargetAddress())) {

                        // sent the correct ACK service now
                        // the ACK info must be sent latest 1,7 ms after receiving the address type octet of an addressed frame
                        _serial.write(TPUART_RX_ACK_SERVICE_ADDRESSED);

                        DEBUG5_PRINTLN(F("assigned to us: src=0x%04x ga=0x%04x"), telegram.getSourceAddress(), telegram.getTargetAddress());

                        _rx.state = RX_KNX_TELEGRAM_RECEPTION_ADDRESSED;

                        // dirty workaround for sending ACk just before reset?
                        // _serial.flush();

                    } else {
                      
                        // sent the correct ACK service now
                        // the ACK info must be sent latest 1,7 ms after receiving the address type octet of an addressed frame
                        _serial.write(TPUART_RX_ACK_SERVICE_NOT_ADDRESSED);

                        DEBUG5_PRINTLN(F("not assigned to us: ga=0x%04x"), telegram.getTargetAddress());
                        
                        _rx.state = RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED;

                        // dirty workaround for sending ACk just before reset?
                        // _serial.flush();
                    }

                    DEBUG5_PRINTLN(F("Size: %d %d"), telegram.getTelegramLength(), telegram.getPayloadLength());
                }
                break;

            case RX_KNX_TELEGRAM_RECEPTION_ADDRESSED:
                DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_ADDRESSED"));
                
            case RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED:
                DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED"));

                if (_rx.readBytes == KNX_TELEGRAM_MAX_SIZE) {
                    DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID"));
                    
                    _rx.state = RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID;
                    rxTaskFinished(telegram);
                    
                } else {
                    DEBUG5_PRINTLN(F("expectedTelegramLength: %d, readBytesNb: %d"),expectedTelegramLength, _rx.readBytes);
                    
                    if (_rx.state == RX_KNX_TELEGRAM_RECEPTION_ADDRESSED) {
                        telegram.setRawByte(incomingByte, _rx.readBytes);
                    }
                    
                    if (expectedTelegramLength == _rx.readBytes) {                        
                        DEBUG5_PRINTLN(F("we are done, telegramCompletelyReceived"));

                        rxTaskFinished(telegram);
                        
                    } else {
                        _rx.readBytes++;
                    }
                }
                break;

            // if the message is too long, just read until expected length
            case RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID:
                _rx.readBytes++;
                break;

            default:
                break;
        }
    }
}

void KnxTpUart::rxTaskFinished(KnxTelegram telegram) {
  
    switch (_rx.state) {

        case RX_KNX_TELEGRAM_RECEPTION_STARTED:
            // we are not supposed to get EOP now, the telegram is incomplete
            DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_STARTED"));
            
        case RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID:
            DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID"));
            
            _evtCallbackFct(TPUART_EVENT_KNX_TELEGRAM_RECEPTION_ERROR);  // Notify telegram reception error
            
            break;

        case RX_KNX_TELEGRAM_RECEPTION_ADDRESSED:
            DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_ADDRESSED ga=0x%04x"), telegram.getTargetAddress());
            
            if (telegram.isChecksumCorrect()) {
                telegram.copy(_rx.receivedTelegram);
                _evtCallbackFct(TPUART_EVENT_RECEIVED_KNX_TELEGRAM);
                
            } else {
                DEBUG5_PRINTLN(F("checksum incorrect."));
                
                _evtCallbackFct(TPUART_EVENT_KNX_TELEGRAM_RECEPTION_ERROR);
            }
            break;

        case RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED:
            DEBUG5_PRINTLN(F("RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED ga=0x%04x"), telegram.getTargetAddress());
            
            break;

        default:
            break;
    }

    // we move state back to RX IDLE in any case
    _rx.state = RX_IDLE_WAITING_FOR_CTRL_FIELD;
    _rx.readBytes = 0;
}

/**
 * Transmission task
 */
void KnxTpUart::txTask(void) {
    word nowTime;
    byte txByte[2];
    static word sentMessageTimeMillisec;

    switch (_tx.state) {

        // STEP 1 : Manage Message Acknowledge timeout
        case TX_WAITING_ACK:
            nowTime = (word)millis();
            
            if (TimeDeltaWord(nowTime, sentMessageTimeMillisec) > KNX_TX_TIMEOUT) {
                DEBUG5_PRINTLN(F("TX_WAITING_ACK Timeout"));
                _tx.state = TX_IDLE;
            }
            break;

        // STEP 2 : send message if any to send
        case TX_TELEGRAM_SENDING_ONGOING:                
            if (_rx.state == RX_IDLE_WAITING_FOR_CTRL_FIELD) {                
                
                // send all available tx bytes at once
                DEBUG5_PRINTLN(F("sending %d bytes index %d"),_tx.bytesRemaining, _tx.txByteIndex);
                while (_tx.bytesRemaining > 0) {
                    txByte[0] = ((_tx.bytesRemaining == 1) ? TPUART_DATA_END_REQ : TPUART_DATA_START_CONTINUE_REQ) + _tx.txByteIndex;
                    txByte[1] = _tx.sentTelegram->getRawByte(_tx.txByteIndex);
                    
                    DEBUG5_PRINTLN(F("data [%d / %d]= %02x %02x"),_tx.txByteIndex, _tx.bytesRemaining, txByte[0], txByte[1]);
                    
                    _serial.write(txByte, 2);  // write the UART control field and the data byte
                    _tx.txByteIndex++;
                    _tx.bytesRemaining--;
                };

                sentMessageTimeMillisec = (word)millis();
                _tx.state = TX_WAITING_ACK;
            }
            break;

        default:
            break;
    }
}

boolean KnxTpUart::isAddressAssigned(word addr) {
    DEBUG5_PRINTLN(F("isAddressAssigned: Searching for 0x%04x %d"), addr, _groupAddressListSize);
  
    for (byte i = 0; i < _groupAddressListSize; i++) {
        DEBUG5_PRINTLN(F("isAddressAssigned: check 0x%04x"), _groupAddressList[i]);
      
        if ( _groupAddressList[i] == addr) {
            DEBUG5_PRINTLN(F("isAddressAssigned: found 0x%04x"), addr);
            return true;
        }
    }

    return false;
}

// Send a KNX telegram
byte KnxTpUart::sendTelegram(KnxTelegram& sentTelegram) {
    DEBUG5_PRINTLN(F("sendTelegram ga=0x%04x"), sentTelegram.getTargetAddress());
    
    sentTelegram.setSourceAddress(_physicalAddr);
    sentTelegram.updateChecksum();
    
    _tx.sentTelegram = &sentTelegram;
    _tx.bytesRemaining = sentTelegram.getTelegramLength();
    _tx.txByteIndex = 0;
    _tx.state = TX_TELEGRAM_SENDING_ONGOING;
                
    return KNX_TPUART_OK;
}
