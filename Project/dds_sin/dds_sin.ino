/*
 * pwmDDS.c
 *
 * Created: 2016-11-13
 * Author : Craig Hollinger
 *
 * Source code for producing a DDS (Direct Digital Synthesis) generated sine
 * wave signal.  We'll use the Fast PWM Mode of Timer/Counter 0 and use an
 * update frequency of 50,000Hz generated by Timer/Counter 2.
 *
 * DDS works by sampling only some of the table of voltage level values
 * (SINE_TABLE) instead of all entries.  By changing how many entries are
 * skipped, the output frequency can be changed.  The more that are skipped, the
 * higher the frequency generated.  The limit of minimum number of samples use
 * (for the highest frequency) is two per cycle of SIN_TABLE as long as they are
 * spread out evenly through the table.  The way DDS works ensures this.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 3
 * or the GNU Lesser General Public License version 3, both as
 * published by the Free Software Foundation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>

/* This is the desired output frequency in Hz. */
#define F_DDS_OUT 2500

/* This is the DDS update frequency in Hz (NOT the PWM frequency). */
#define F_UPDATE 50000

/* Sine table, from 0 to 255.  Each value represents a voltage level on a
   sinusoidal waveform.
   This uses up a lot of RAM (256 bytes) because this is where it is stored.
   Could be moved to FLASH, but would require more overhead to get the data out. */
uint8_t SINE_TABLE[]=
{
  128, 131, 134, 137, 140, 143, 146, 149, 152, 156, 159, 162, 165, 168, 171, 174,
  176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 206, 209, 211, 213, 216,
  218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 240, 242, 243, 245,
  246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 251, 250, 249, 248, 247,
  246, 245, 243, 242, 240, 239, 237, 236, 234, 232, 230, 228, 226, 224, 222, 220,
  218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 191, 188, 185, 182, 179,
  176, 174, 171, 168, 165, 162, 159, 156, 152, 149, 146, 143, 140, 137, 134, 131,
  127, 124, 121, 118, 115, 112, 109, 106, 103, 99, 96, 93, 90, 87, 84, 81,
  79, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39,
  37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 18, 16, 15, 13, 12, 10,
  9, 8, 7, 6, 5, 4, 3, 3, 2, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 3, 4, 5, 6, 7, 8,
  9, 10, 12, 13, 15, 16, 18, 19, 21, 23, 25, 27, 29, 31, 33, 35,
  37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76,
  79, 81, 84, 87, 90, 93, 96, 99, 103, 106, 109, 112, 115, 118, 121, 124
};

/* These are the counters used to generate the index into SINE_TABLE.  They are
   16-bits long here, but could be just 8.  Eight-bit registers will work but
   more bits give better frequency resolution. */
uint16_t phaseReg, phaseInc;

/* This is the Timer 2 Compare A interrupt service routine.  Runs every time
   the counter (TCNT0) matches the output compare register A (OCR0A).  Here a
   new value is read from SINE_TABLE and written the output compare register. */
ISR(TIMER2_COMPA_vect)
{
  uint8_t i;

  phaseReg += phaseInc;
  i = (uint8_t)(phaseReg >> 8); /* use only the upper 8-bits */

  OCR0A = SINE_TABLE[i]; /* update TC0 output compare register */

}/* end ISR(TIMER2_COMPA_vect) */

/* This is where it all happens. */
int main(void)
{
  /* Set up the IO port registers for the IO pin connected to the PWM output
     pin OC0A (PD6), Arduino pin 6. */
  DDRD |= (_BV(PORTD6)) | (_BV(PORTD5));

  /* Set up Timer 0 to generate a Fast PWM signal:
     - clocked by F_CPU (fastest PWM frequency)
     - non-inverted output */
  TCCR0A = (_BV(WGM00) | _BV(WGM01) | _BV(COM0A1)); /* TC0 Mode 3, Fast PWM */
  TCCR0B = _BV(CS00); /* TC0 clocked by F_CPU, no prescale */
  OCR0A = 0; /* start with duty cycle = 0% */

  /* Set up Timer 2 to interrupt at 50,000Hz:
     - clocked by F_CPU / 8
     - generate interrupt when OCR2A matches TCNT2
     - load OCR2A with the count to get 50,000Hz
   */
  TCCR2A = _BV(WGM21); /* TC2 mode 2, CTC - clear timer on match A */
  TCCR2B = _BV(CS21); /* clock by F_CPU / 8 */
  OCR2A = (F_CPU / 8 / F_UPDATE - 1); /* = 39 */
  TIMSK2 = _BV(OCIE2A); /* enable OCIE2A, match A interrupt */

  phaseReg = 0;
  phaseInc = F_DDS_OUT * 65536 / F_UPDATE;

  /* enable the interrupt system */
  sei();

  /* run around this loop for ever */
  while (1) 
  {
    /* Nothing happens here, all the work is done in the ISR */
  }/* end while(1) */

}/* end main() */
