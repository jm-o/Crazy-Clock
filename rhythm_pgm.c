/*

 Rhythm Clock for Arduino
 Copyright 2016 Jean Michel Olivier
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * This code reads a sequence of sleep times (in tenths-of-a-second) from eeprom.
 * The sequence duration can't be greater than 60 seconds.
 *
 * To insure that the correct ticking frequency, 60 ticks per minute, is maintained:
 * - 60 modulo (sleep times count + 1) MUST be equal to 0
 * - The sum of sleep times + wait time MUST be equal to (sleep times count + 1) * 10
 *
 * To change pattern, modify:
 * sleep[] : sleep times (1 byte each - Maximum count 59)
 * WAIT : wait time between each sequence of sleep times
 *
 */

#include "base.h"
#include <avr/pgmspace.h>

// Sleep times (1 byte each - Maximum count 59)
const unsigned char sleep[] PROGMEM = {0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
                                       0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
                                       0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
                                       0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
                                       0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
                                       0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A};

// Wait time between each sequence of sleep times (unsigned int)
#define WAIT (0x000A)

void loop() {

  unsigned char i;
  unsigned int j;

  while(1) {
    for(i = 0; i < sizeof(sleep); i++) {
      doTick();
      for(j = 1; j < pgm_read_byte(sleep[i]); j++)
        doSleep();
    }
    doTick();
    for(j = 1; j < WAIT; j++)
      doSleep();
  }
}
