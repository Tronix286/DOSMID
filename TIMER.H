/*
 * This file is part of the DOSMid project.
 * Copyright (C) Mateusz Viste 2014-2018
 *
 * This file contains the implementation of a more precise timer than that
 * provided by DOS. Routines are provided to increase the PIT clock rate to
 * around 1165 interrupts per second, for a granularity of close to 858
 * microseconds between clock pulses, rather than the 55 milliseconds between
 * standard PC clock pulses (18.2 times/second).
 *
 * Note that the timer_start() routine must be called before the timer_read()
 * routines will work, and that the timer_stop() routine MUST be called
 * before the program terminates, or the machine will be toasted. For this
 * reason, timer_init() installs the timer_stop() routine to be called at
 * program exit.
 */

#ifndef timer_h_sentinel
#define timer_h_sentinel

/* reset the timer value, this can be used by the application to make sure
 * no timer wrap occurs during critical parts of the code flow */
void timer_reset(void);

/* This routine will stop the fast clock if it is going. It has void return
 * value so that it can be an exit procedure. */
void timer_stop(void);

/* This routine will start the fast clock rate by installing the
 * handle_clock routine as the interrupt service routine for the clock
 * interrupt and then setting the interrupt rate up to its higher speed
 * by programming the 8253 timer chip.
 * This routine does nothing if the clock rate is already set to
 * its higher rate, but then it returns -1 to indicate the error. */
void timer_init(void);

/* This routine will return the present value of the time, as a number of
 * microseconds. Interrupts are disabled during this time to prevent the
 * clock from changing while it is being read. */
void timer_read(unsigned long *res);

/* high resolution sleeping routine, waits n microseconds */
void udelay(unsigned long us);

#endif
