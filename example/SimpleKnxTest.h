/*
 *    SimpleKnxTest.h
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

#ifndef _SimpleKnxTest_H_
#define _SimpleKnxTest_H_

//add your includes for the project SimpleKnxTest here
#include "Arduino.h"
#include <DebugUtil.h>
#include <SimpleKnx.h>
//end of add your includes here

//add your function definitions for the project SimpleKnxTest here
void setup();
void loop();
void telegramEventCallback(KnxTelegram telegram);
void telegramEventRead(KnxTelegram telegram);
void telegramEventWrite(KnxTelegram telegram);

//Do not add code below this line
#endif /* _SimpleKnxTest_H_ */
