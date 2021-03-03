/*
 * This file contains the implementation of a more precise time than that
 * provided by DOS.  Routines are provided to increase the clock rate to
 * around 1165 interrupts per second, for a granularity of close to 858
 * microseconds between clock pulses, rather than the 55 milliseconds between
 * normal PC clock pulses (18.2 times/second).
 *
 * Note that the timer_init() routine must be called before the timer_read()
 * routines will work, and that the timer_stop() routine MUST be called
 * before the program terminates, or the machine will be toasted. For this
 * reason, timer_init() installs the timer_stop() routine to be called at
 * program exit.
 */

#include <stdlib.h> /* atexit() */
#include <dos.h>   /* _chain_intr(), _disable(), _enable(), _dos_setvect() */
#include <conio.h> /* outp() */
#include "timer.h" /* include self for control */

/* selects timer's resolution */
#define TIMER_1165_HZ
/*#define TIMER_582_HZ*/
/*#define TIMER_291_HZ*/

/* 1165.215 hz */
#ifdef TIMER_1165_HZ
#define DIV_MSB   0x04 /* a divisor of 1024 gives: */
#define DIV_LSB   0x00 /* 1193180 / 1024 = 1165 Hz */
#define ORIG_INTR_MASK 63 /* call the original clock INT every 64 calls */
#define USEC_INC  858 /* one cycle is 858.21us to be exact */
#define USEC_COMP 13  /* how many us to compensate for every ORIG_INTR_MASK+1 call */
#endif

/* 582.607 hz */
#ifdef TIMER_582_HZ
#define DIV_MSB   0x08
#define DIV_LSB   0x00
#define ORIG_INTR_MASK 31
#define USEC_INC  1716 /* 1716.42 to be exact */
#define USEC_COMP 13   /* how many us to compensate for every ORIG_INTR_MASK+1 call */
#endif

/* 291.304 hz */
#ifdef TIMER_291_HZ
#define DIV_MSB   0x10
#define DIV_LSB   0x00
#define ORIG_INTR_MASK 15
#define USEC_INC  3433 /* 3432.84 to be exact */
#define USEC_COMP -2
#endif


#define CLOCK_INT 0x08

/* redefine a few functions, as needed by OpenWatcom */
#define setvect _dos_setvect
#define getvect _dos_getvect
#define disable _disable
#define enable _enable

static unsigned long nowtime = 0;        /* current time counter */
static void interrupt (*oldfunc)(void);  /* interrupt function pointer */


/* This routine will handle the clock interrupt at its higher rate. It will
 * call the DOS handler every ORIG_INTR_MASK times it is called, to maintain
 * the 18.2 times per second that DOS needs to be called. Each time through,
 * it adds to the nowtime value.
 * When it is not calling the DOS handler, this routine must reset the 8259A
 * interrupt controller before returning. */
static void interrupt handle_clock(void) {
  static int callmod = 0;

  /* increment the time */
  nowtime += USEC_INC;

  /* increment the callmod */
  callmod++;
  callmod &= ORIG_INTR_MASK;

  /* if this is the 64th call, then call handler */
  if (callmod == 0) {
    nowtime += USEC_COMP; /* compensate for integer division inaccuracy */
    _chain_intr(oldfunc);
  } else {  /* otherwise, clear the interrupt controller */
    outp(0x20, 0x20);  /* end of interrupt */
  }
}


/* reset the timer value, this can be used by the application to make sure
 * no timer wrap occurs during critical parts of the code flow */
void timer_reset(void) {
  disable();
  nowtime = 0;
  enable();
}


/* This routine will stop the timer. It has void return value so that it
 * can be an exit procedure. */
void timer_stop(void) {
  /* Disable interrupts */
  disable();

  /* Reinstate the old interrupt handler */
  setvect(CLOCK_INT, oldfunc);

  /* Reinstate the clock rate to standard 18.2 Hz */
  outp(0x43, 0x36);       /* Set up for count to be sent          */
  outp(0x40, 0x00);       /* LSB = 00  \_together make 65536 (0)  */
  outp(0x40, 0x00);       /* MSB = 00  /                          */

  /* Enable interrupts */
  enable();
}


/* This routine will start the fast clock rate by installing the handle_clock
 * routine as the interrupt service routine for the clock interrupt and then
 * setting the interrupt rate up to its higher speed by programming the 8253
 * timer chip. */
void timer_init(void) {
  /* Store the old interrupt handler */
  oldfunc = getvect(CLOCK_INT);

  /* Set the nowtime to zero */
  nowtime = 0;

  /* Disable interrupts */
  disable();

  /* Install the new interrupt handler */
  setvect(CLOCK_INT, handle_clock);

  /* Increase the clock rate */
  outp(0x43, 0x36);     /* Set up for count to be sent            */
  outp(0x40, DIV_LSB);  /* LSB = 00  \_together make 2^10 = 1024  */
  outp(0x40, DIV_MSB);  /* MSB = 04  /                            */

  /* Enable interrupts */
  enable();

  /* Install the timer_stop() routine to be called at exit */
  atexit(timer_stop);
}


/* This routine will return the present value of the time, as a number of
 * microseconds. Interrupts are disabled during this time to prevent the
 * clock from changing while it is being read. */
void timer_read(unsigned long *res) {
  /* Disable interrupts */
  disable();

  /* Read the time */
  *res = nowtime;

  /* Enable interrupts */
  enable();
}


/* high resolution sleeping routine, waits n microseconds */
void udelay(unsigned long us) {
  unsigned long t1, t2;
  timer_read(&t1);
  for (;;) {
    timer_read(&t2);
    if (t2 < t1) { /* detect timer wraparound */
      break;
    } else if (t2 - t1 >= us) {
      break;
    }
  }
}
