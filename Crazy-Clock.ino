/*

 Crazy Clock for Arduino
 Copyright 2014 Nicholas W. Sayer
 
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
 * This is intended to run on an ATTiny85. Connect a 16.384 MHz crystal and fuse it
 * for divide-by-8 clocking, no watchdog or brown-out detector.
 *
 * Connect PB0 and PB1 to the coil pins of a Lavet stepper coil of a clock movement
 * (with a series resistor and flyback diode to ground on each pin) and power it 
 * from a 3.3 volt boost converter.
 *
 * It will keep a long-term average pulse rate of 1 Hz (alternating coil pins), but
 * will interleve periods of double-time and half-time ticking.
 *
 */
 
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

// clock solenoid pins
#define P0 0
#define P1 1
#define P_UNUSED 2

// Update the PRNG seed daily
#define SEED_UPDATE_INTERVAL (86400)

// We're going to set up a timer interrupt for every 100 msec, so what's 1/.1?
// Note that while we're actually doing stuff, we *must* insure that we never
// work through an interrupt. This is because we're not *counting* these
// interrupts, we're just waiting for each one in turn.
#define IRQS_PER_SECOND (10)

// How long is each tick? In this case, we're going to busy-wait on the timer.
#define TICK_LENGTH (35)

#define TICK_PIN (lastTick == P0?P1:P0)

unsigned char lastTick;

#define HALF_SPEED 1
#define NORMAL_SPEED 2
#define DOUBLE_SPEED 3

// This *must* be even!
#define LIST_LENGTH 14

unsigned char instruction_list[LIST_LENGTH];

unsigned char place_in_list;

unsigned char time_per_step;

unsigned char time_in_step;

unsigned char half_tick_placeholder;

// This delay loop is magical because we know the timer is ticking at 1 kHz.
// So we just wait until it counts N times and that will be an N msec delay.
static void delay_ms(unsigned char msec) {
   unsigned char start_time = TCNT0;
   while(TCNT0 - start_time < msec / 2) ; // sit-n-spin
}

static void doTick() {
  digitalWrite(TICK_PIN, HIGH);
  delay_ms(TICK_LENGTH);
  digitalWrite(TICK_PIN, LOW);
  lastTick = TICK_PIN;
  sleep_mode(); // eat the rest of this tick
}

static void updateSeed() {
  unsigned long seed = random();
  EEPROM.write(0, (char)seed);
  EEPROM.write(1, (char)(seed >> 8));
  EEPROM.write(2, (char)(seed >> 16));
  EEPROM.write(3, (char)(seed >> 24));
}

ISR(TIMER0_COMPA_vect) {
  // do nothing - just wake up
}

void setup() {
  clock_prescale_set(clock_div_32);
  power_adc_disable();
  power_usi_disable();
  power_timer1_disable();
  //PRR = _BV(PRADC) | _BV(PRTIM1) | _BV(PRUSI); // everything off but timer 0.
  TCCR0A = _BV(WGM01); // mode 2 - CTC
  TCCR0B = _BV(CS02) | _BV(CS00); // prescale = 1024
  // xtal freq = 16.384 MHz.
  // CPU freq = 16.384 MHz / 32 = 512 kHz
  // count freq = 512 kHz / 1024 = 500 Hz
  OCR0A = 50; // 10 Hz
  TIMSK = _BV(OCIE0A); // OCR0A interrupt only.
  ACSR = _BV(ACD); // Turn off analog comparator - but was it ever on anyway?
  
  set_sleep_mode(SLEEP_MODE_IDLE);

  pinMode(P_UNUSED, INPUT_PULLUP);
  pinMode(P0, OUTPUT);
  pinMode(P1, OUTPUT);
  digitalWrite(P0, LOW);
  digitalWrite(P1, LOW);
  
  lastTick = P0;
  place_in_list = LIST_LENGTH; // force a reset.
    
  // Try and perturb the PRNG as best as we can
  unsigned long seed = EEPROM.read(0);
  seed |= ((unsigned long)EEPROM.read(1))<<8;
  seed |= ((unsigned long)EEPROM.read(2))<<16;
  seed |= ((unsigned long)EEPROM.read(3))<<24;
  randomSeed(seed);
  updateSeed();
}

void loop() {
  unsigned long seedUpdateAfter = SEED_UPDATE_INTERVAL;
  while(1){
    // The intent is for the top of this loop to be hit once per second
    if (--seedUpdateAfter == 0) {
      updateSeed();
      seedUpdateAfter = SEED_UPDATE_INTERVAL;
    }
    if (place_in_list >= LIST_LENGTH) {
      // We're out of instructions. Time to make some.
      for(int i = 0; i < LIST_LENGTH / 2; i++) {
        // We're going to add instructions in pairs - either a double-and-half time pair or a pair of normals.
        // Adding the half and double speed in pairs - even if they're not done adjacently (as long as they *do* get done)
        // will insure the clock will keep long-term time accurately.
        switch(random(2)) {
          case 0: instruction_list[2 * i] = HALF_SPEED;
                  instruction_list[2 * i + 1] = DOUBLE_SPEED;
                  break;
          case 1: instruction_list[2 * i] = NORMAL_SPEED;
                  instruction_list[2 * i + 1] = NORMAL_SPEED;
                  break;
        }
      }
      // Now shuffle the array
      for(int i = 0; i < LIST_LENGTH; i++) {
        unsigned char swapspot = random(LIST_LENGTH - i);
        unsigned char temp = instruction_list[i];
        instruction_list[i] = instruction_list[swapspot];
        instruction_list[swapspot] = temp;
      }
      // This must be even!
      // It also should be long enough to establish a pattern
      // before changing.
      time_per_step = random(5) * 2 + 10;
      place_in_list = 0;
      time_in_step = 0;
    }
    
      // What are we doing right now?
    switch(instruction_list[place_in_list]) {
      case HALF_SPEED:
        if (half_tick_placeholder) {
          // do nothing, but take the time that a tick would take.
          sleep_mode();
        } else {
          doTick();
        }
        for(int i = 0; i < IRQS_PER_SECOND - 1; i++)
          sleep_mode();
        half_tick_placeholder = !half_tick_placeholder;
        break;
      case NORMAL_SPEED:
        doTick();
        for(int i = 0; i < IRQS_PER_SECOND - 1; i++)
          sleep_mode();
        break;
      case DOUBLE_SPEED:
        for(int i = 0; i < 2; i++) {
          doTick();
          for(int j = 0; j < 4; j++)
            sleep_mode();
        }
        break;
    }
    if (time_in_step++ >= time_per_step) {
      time_in_step = 0;
      place_in_list++;
    }
  }
}
