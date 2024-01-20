/*
 *    DebugUtil.cpp
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

#ifndef DEBUGUTIL_H
#define DEBUGUTIL_H


#if defined(DEBUGGING_0) || defined(DEBUGGING_1) || defined(DEBUGGING_2) || defined(DEBUGGING_3) || defined(DEBUGGING_4) || defined(DEBUGGING_5)
#define DEBUGGING
#define DEBUG_PRINT(...) Debug.print(__VA_ARGS__);
#define DEBUG_PRINTLN(...) Debug.println(__VA_ARGS__);
#endif


#ifdef DEBUGGING_0
#define DEBUG0_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG0_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG0_PRINT(...)
#define DEBUG0_PRINTLN(...)
#endif

#ifdef DEBUGGING_1
#define DEBUG1_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG1_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG1_PRINT(...)
#define DEBUG1_PRINTLN(...)
#endif

#ifdef DEBUGGING_2
#define DEBUG2_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG2_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG2_PRINT(...)
#define DEBUG2_PRINTLN(...)
#endif

#ifdef DEBUGGING_3
#define DEBUG3_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG3_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG3_PRINT(...)
#define DEBUG3_PRINTLN(...)
#endif

#ifdef DEBUGGING_4
#define DEBUG4_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG4_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG4_PRINT(...)
#define DEBUG4_PRINTLN(...)
#endif

#ifdef DEBUGGING_5
#define DEBUG5_PRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DEBUG5_PRINTLN(...) DEBUG_PRINTLN(__VA_ARGS__)
#else
#define DEBUG5_PRINT(...)
#define DEBUG5_PRINTLN(...)
#endif



#ifdef DEBUGGING
#include <Arduino.h>
#include <SoftwareSerial.h>

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  ((byte & 0x80) ? 1 : 0), \
  ((byte & 0x40) ? 1 : 0), \
  ((byte & 0x20) ? 1 : 0), \
  ((byte & 0x10) ? 1 : 0), \
  ((byte & 0x08) ? 1 : 0), \
  ((byte & 0x04) ? 1 : 0), \
  ((byte & 0x02) ? 1 : 0), \
  ((byte & 0x01) ? 1 : 0) 

class DebugUtil {
    
private:

    // Constructor, Destructor
    DebugUtil();            // private constructor (singleton design pattern)
    ~DebugUtil() {}         // private destructor (singleton design pattern)
    DebugUtil(DebugUtil&);  // private copy constructor (singleton design pattern)
    
    Print* _printstream;

    void init(void);
    void setPrintStream(Stream* printstream);

public:
    static DebugUtil Debug;
    
    void print(const char *format, ...);
    void print(const __FlashStringHelper *format, ...);
    void println(const char *format, ...);
    void println(const __FlashStringHelper *format, ...);

};

extern DebugUtil& Debug;

#endif

#endif // DEBUGUTIL_H
