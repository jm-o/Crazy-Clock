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
 * This is intended to run on an ATTiny45. Connect a 4.00 MHz, 4.096 MHz or a 32.768 kHz
 * crystal and fuse it for the appropriate oscillator (set the divide-by-8 fuse for 4.x MHz),
 * no watchdog or brown-out detector.
 *
 * Connect PB0 and PB1 to the coil pins of a Lavet stepper coil of a clock movement
 * (with a series resistor and flyback diode to ground on each pin) and power it 
 * from a 3.3 volt boost converter.
 *
 * This file is the common infrastructure for all of the different clock types.
 * It sets up a 10 Hz interrupt. The clock code(s) keep accurate time by calling
 * either doTick() or doSleep() repeatedly. Each method will put the CPU to sleep
 * until the next tenth-of-a-second interrupt (tick() will tick the clock once first).
 * In addition to doTick() and doSleep(), any clock code that makes use of random()
 * should occasionally (SEED_UPDATE_INTERVAL) make a call to updateSeed(). That will
 * update the PRNG seed value stored in EEPROM, which insures that the clock doesn't
 * repeat its previous behavior every time you change the battery.
 *
 * The clock code should insure that it doesn't do so much work that works through
 * a 10 Hz interrupt interval. Every time that happens, the clock loses a tenth of
 * a second. In particular, generating random numbers is a costly operation.
 *
 */

#include <stdlib.h> 
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#include "base.h"

#if !(defined(FOUR_MHZ_CLOCK) ^ defined(THIRTYTWO_KHZ_CLOCK))
#error Must pick either 4 MHz or 32 kHz option.
#endif

#if defined(FOUR_MHZ_CLOCK)
// 4,000,000 divided by 128 is 31,250.
// 31,250 divided by (64 * 10) is a divisor of 48 53/64, which is 49*53 + 48*11
#define CLOCK_CYCLES (64)
// Don't forget to decrement the OCR0A value - it's 0 based and inclusive
#define CLOCK_BASIC_CYCLE (48 - 1)
// a "long" cycle is CLOCK_BASIC_CYCLE + 1
#define CLOCK_NUM_LONG_CYCLES (53)
#elif defined(THIRTYTWO_KHZ_CLOCK)
// 32,768 divided by (64 * 10) yields a divisor of 51 1/5, which is 52 + 51*4
#define CLOCK_CYCLES (5)
// Don't forget to decrement the OCR0A value - it's 0 based and inclusive
#define CLOCK_BASIC_CYCLE (51 - 1)
// a "long" cycle is CLOCK_BASIC_CYCLE + 1
#define CLOCK_NUM_LONG_CYCLES (1)
#endif

// clock solenoid pins
#define P0 0
#define P1 1
#define P_UNUSED 2

// How long is each tick? In this case, we're going to busy-wait on the timer.
#define TICK_LENGTH (35)

// This delay loop is magical because we know the timer is ticking at approximately 500 Hz.
// So we just wait until it counts N/2 times and that will be an N msec delay.
// This will be a little off, but this is not a critical timing interval.
static void delay_ms(unsigned char msec) {
   unsigned char start_time = TCNT0;
   while(TCNT0 - start_time < msec / 2) ; // sit-n-spin
}

void doSleep() {
  static unsigned char cycle_pos = 0xfe; // force a reset

  if (++cycle_pos == CLOCK_NUM_LONG_CYCLES)
    OCR0A = CLOCK_BASIC_CYCLE;
  if (cycle_pos >= CLOCK_CYCLES) {
    OCR0A = CLOCK_BASIC_CYCLE + 1;
    cycle_pos = 0;
  }
  sleep_mode();
}

// This will alternate the ticks
#define TICK_PIN (lastTick == P0?P1:P0)

// Each call to doTick() will "eat" a single one of our interrupt "ticks"
void doTick() {
  static unsigned char lastTick = P0;

  PORTB |= _BV(TICK_PIN);
  delay_ms(TICK_LENGTH);
  PORTB &= ~ _BV(TICK_PIN);
  lastTick = TICK_PIN;
  doSleep(); // eat the rest of this tick
}

void updateSeed() {
  unsigned long seed = random();
  eeprom_write_dword(0, seed);
}

ISR(TIMER0_COMPA_vect) {
  // do nothing - just wake up
  _NOP();
}

extern void loop();

void main() {
#ifndef THIRTYTWO_KHZ_CLOCK
  // change this so that we wind up with as near a 32 kHz CPU clock as possible.
  clock_prescale_set(clock_div_128);
#endif
  ADCSRA = 0; // DIE, ADC!!! DIE!!!
  ACSR = _BV(ACD); // Turn off analog comparator - but was it ever on anyway?
  power_adc_disable();
  power_usi_disable();
  power_timer1_disable();
  TCCR0A = _BV(WGM01); // mode 2 - CTC
  TCCR0B = _BV(CS01) | _BV(CS00); // prescale = 64
  TIMSK = _BV(OCIE0A); // OCR0A interrupt only.
  
  set_sleep_mode(SLEEP_MODE_IDLE);

  DDRB = _BV(P0) | _BV(P1) | _BV(P_UNUSED); // all our pins are output.
  PORTB = 0; // Initialize all pins low.
      
  // Try and perturb the PRNG as best as we can
  unsigned long seed = eeprom_read_dword(0);
  srandom(seed);
  updateSeed();

  // Don't forget to turn the interrupts on.
  sei();

  // Now hand off to the specific clock code
  while(1) loop();

}

