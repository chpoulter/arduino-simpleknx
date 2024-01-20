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

#include "DebugUtil.h"

#ifdef DEBUGGING

#include <SoftwareSerial.h>
SoftwareSerial debugSerial(9, 8);

DebugUtil DebugUtil::Debug;
DebugUtil& Debug = DebugUtil::Debug;

/*
 * Format Help:
 * http://www.cplusplus.com/reference/cstdio/printf/
 * 
 * %i = signed decimal integer
 * %u = unsigned decimal integer
 * %x = hex
 * %X = upper case hex
 * %s = string
 * %c = character
 * 0x%02x = hex representation like 0xff
 * %% = % symbol
 */
DebugUtil::DebugUtil() {
	init();
}

void DebugUtil::init(void) {
	debugSerial.begin(19200);
	setPrintStream(&debugSerial);
}

void DebugUtil::setPrintStream(Stream* printstream) {
    _printstream = printstream;
}

void DebugUtil::print(const char *format, ...) {
    if (_printstream) {

    	char buf[128]; // limit to 128chars
        va_list args;
        va_start(args, format);

        vsnprintf(buf, 128, format, args);

        va_end(args);
        _printstream->print(buf);
    }

}

void DebugUtil::print(const __FlashStringHelper *format, ...) {
    if (_printstream) {

        char buf[128]; // limit to 128chars
        va_list args;
        va_start(args, format);

        vsnprintf_P(buf, sizeof (buf), (const char *) format, args); // progmem for AVR and ESP8266

        va_end(args);
        _printstream->print(buf);
    }
}

void DebugUtil::println(const char *format, ...) {
    if (_printstream) {

        char buf[128]; // limit to 128chars
        va_list args;
        va_start(args, format);

        vsnprintf(buf, 128, format, args);

        va_end(args);
        _printstream->println(buf);
    }
}

void DebugUtil::println(const __FlashStringHelper *format, ...) {
    if (_printstream) {

        char buf[128]; // limit to 128chars
        va_list args;
        va_start(args, format);

        vsnprintf_P(buf, sizeof (buf), (const char *) format, args); // progmem for AVR and ESP8266

        va_end(args);
        _printstream->println(buf);
    }
}

#endif
