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
 * See rhythm.md to change the pattern
 *
 */

// EEPROM locations
#define EE_COUNT_LOC ((void*)6)
#define EE_WAIT_LOC ((void*)7)
#define EE_SLEEP_LOC ((void*)9)

// Max number of sleep times
#define MAX_SLEEPS (59)

#include "base.h"
#include <avr/eeprom.h>

void loop() {

  unsigned char sleep[MAX_SLEEPS];
  unsigned char i;
  unsigned int j;

  unsigned char count = (unsigned char)eeprom_read_byte(EE_COUNT_LOC);
  if (count > MAX_SLEEPS) count = MAX_SLEEPS;

  unsigned int wait = (unsigned int)eeprom_read_word(EE_WAIT_LOC);
  
  for(i = 0; i < count; i++)
    sleep[i] = (unsigned char)eeprom_read_byte(EE_SLEEP_LOC + i);

  while(1) {
    for(i = 0; i < count; i++) {
      doTick();
      for(j = 1; j < sleep[i]; j++)
        doSleep();
    }
    doTick();
    for(j = 1; j < wait; j++)
      doSleep();
  }
}
