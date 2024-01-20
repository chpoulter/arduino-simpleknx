/*
 *    KnxTpUart.h
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

#ifndef KNXTPUART_H
#define KNXTPUART_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "KnxTelegram.h"

// Values returned by the KnxTpUart member functions :
#define KNX_TPUART_OK                            0
#define KNX_TPUART_ERROR                       255
#define KNX_TPUART_ERROR_NOT_INIT_STATE        254

// Services to TPUART (hostcontroller -> TPUART) :
#define TPUART_RESET_REQ                     0x01
#define TPUART_STATE_REQ                     0x02
#define TPUART_SET_ADDR_REQ                  0x28
#define TPUART_DATA_START_CONTINUE_REQ       0x80
#define TPUART_DATA_END_REQ                  0x40
#define TPUART_ACTIVATEBUSMON_REQ            0x05
#define TPUART_RX_ACK_SERVICE_ADDRESSED      0x11
#define TPUART_RX_ACK_SERVICE_NOT_ADDRESSED  0x10

// Services from TPUART (TPUART -> hostcontroller) :
#define TPUART_RESET_INDICATION               0x03
#define TPUART_DATA_CONFIRM_SUCCESS           0x8B
#define TPUART_DATA_CONFIRM_FAILED            0x0B
#define TPUART_STATE_INDICATION               0x07
#define TPUART_STATE_INDICATION_MASK          0x07
#define KNX_CONTROL_FIELD_PATTERN_MASK   0b11010011 // 0xD3
#define KNX_CONTROL_FIELD_VALID_PATTERN  0b10010000 // 0x90
#define KNX_PAYLOAD_LENGTH_MASK          0b00001111 // 0x0F

// Mask for STATE INDICATION service
#define TPUART_STATE_INDICATION_SLAVE_COLLISION_MASK  0x80
#define TPUART_STATE_INDICATION_RECEIVE_ERROR_MASK    0x40
#define TPUART_STATE_INDICATION_TRANSMIT_ERROR_MASK   0x20
#define TPUART_STATE_INDICATION_PROTOCOL_ERROR_MASK   0x10
#define TPUART_STATE_INDICATION_TEMP_WARNING_MASK     0x08

// Timeouts
#define KNX_RX_TIMEOUT 50000 // us
#define KNX_TX_TIMEOUT 500   // ms


// --- Definitions for the RECEPTION  part ----
// Definition of the TP-UART events sent to the application layer
enum KnxTpUartEvent { 
    TPUART_EVENT_RESET = 0,                          // 0: reset received from the TPUART device
    TPUART_EVENT_RECEIVED_KNX_TELEGRAM = 1,          // 1: a new addressed KNX Telegram has been received
    TPUART_EVENT_KNX_TELEGRAM_RECEPTION_ERROR = 2,   // 2: a new addressed KNX telegram reception failed
 };

// RX states
enum TpUartRxState {
    RX_RESET = 0,                                 // The RX part is awaiting reset execution
    RX_STOPPED = 1,                               // TPUART reset event received, RX activity is stopped
    RX_INIT = 2,                                  // The RX part is awaiting init execution
    RX_IDLE_WAITING_FOR_CTRL_FIELD = 3,           // Idle, no reception ongoing
    RX_KNX_TELEGRAM_RECEPTION_STARTED = 4,        // Telegram reception started (address evaluation not done yet)
    RX_KNX_TELEGRAM_RECEPTION_ADDRESSED = 5,      // Addressed telegram reception ongoing
    RX_KNX_TELEGRAM_RECEPTION_LENGTH_INVALID = 6, // The telegram being received is too long
    RX_KNX_TELEGRAM_RECEPTION_NOT_ADDRESSED = 7   // Tegram reception ongoing but not addressed
};

typedef struct TpUartRx {
    byte readBytes;
    bool telegramCompletelyReceived;   // receiving telegram finished
    TpUartRxState state;               // Current TPUART RX state
    KnxTelegram receivedTelegram;      // Where each received telegram is stored (the content is overwritten on each telegram reception)

} TpUartRx;

// Typedef for events callback function
typedef void (*EventCallbackFctPtr) (KnxTpUartEvent);


// --- Definitions for the TRANSMISSION  part ----
// Transmission states
enum TpUartTxState {
    TX_RESET = 0,                     // The TX part is awaiting reset execution
    TX_STOPPED = 1,                   // TPUART reset event received, TX activity is stopped
    TX_INIT = 2,                      // The TX part is awaiting init execution
    TX_IDLE = 3,                      // Idle, no transmission ongoing
    TX_TELEGRAM_SENDING_ONGOING = 4,  // KNX telegram transmission ongoing
    TX_WAITING_ACK = 5                // Telegram transmitted, waiting for ACK/NACK
};

typedef struct TpUartTx {
    TpUartTxState state;              // Current TPUART TX state
    KnxTelegram *sentTelegram;        // Telegram being sent
    byte bytesRemaining;              // Nb of bytes remaining to be transmitted
    byte txByteIndex;                 // Index of the byte to be sent
} TpUartTx;

class KnxTpUart {
    HardwareSerial& _serial;                  
    TpUartRx _rx;                       
    TpUartTx _tx;                       
    EventCallbackFctPtr _evtCallbackFct; 
    const word _physicalAddr;                 
    const word *_groupAddressList;
    const byte _groupAddressListSize;   

  public:  
    KnxTpUart(HardwareSerial& serial, word physicalAddr, word groupAddressList[], byte groupAddressListSize);
    ~KnxTpUart();

    byte init(void);
    byte reset(void);
    byte setEvtCallback(EventCallbackFctPtr);

    boolean isActive(void) const;
    boolean isFreeToSend(void) const;
    boolean isRxActive(void) const;    

    void rxTask(void);
    void txTask(void);

    KnxTelegram& getReceivedTelegram(void);
    byte sendTelegram(KnxTelegram& sentTelegram);

  private:
    void rxTaskFinished(KnxTelegram telegram);
    boolean isAddressAssigned(word addr);
};


// ----- Definition of the INLINED functions :  ------------
inline KnxTelegram& KnxTpUart::getReceivedTelegram(void) { return _rx.receivedTelegram; }
inline boolean KnxTpUart::isActive(void) const { return ( _rx.state > RX_IDLE_WAITING_FOR_CTRL_FIELD) || ( _tx.state > TX_IDLE); }
inline boolean KnxTpUart::isFreeToSend(void) const { return ( _rx.state == RX_IDLE_WAITING_FOR_CTRL_FIELD) && ( _tx.state == TX_IDLE); }
inline boolean KnxTpUart::isRxActive(void) const { return ( _rx.state > RX_IDLE_WAITING_FOR_CTRL_FIELD); }

#endif // KNXTPUART_H
