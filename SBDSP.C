/*
 * SoundBlaster DSP code
 * This file is part of the DOSMid project
 * Copyright (C) Mateusz Viste 2015
 */

#include <conio.h>  /* inp() and outp() */

#include "timer.h"

#include "sbdsp.h" /* include self for control */


/* convenience defines for the I/O addresses of the SoundBlaster */
#define IO_RESET  (port+0x06)
#define IO_READ   (port+0x0A)
#define IO_WRITE  (port+0x0C)
#define IO_WRSTAT (port+0x0C)
#define IO_RDSTAT (port+0x0E)


int dsp_reset(unsigned short port) {
  /* 1. Write a "1" to the Reset port (2x6h) and wait for 3 microseconds.
   * 2. Write a "0" to the Reset port.
   * 3. Poll for a ready byte 0AAh from the Read Data port. You must check the
   *    the Read-Buffer Status port to ensure there is data before reading the
   *    Read Data port. */
  int x, timeout;
  outp(IO_RESET, 1);
  udelay(2000); /* wait 2 ms */
  outp(IO_RESET, 0);
  /* wait for the DSP to give us the green light - no longer than 50ms (the
   * SoundBlaster docs state that the DSP should init in about 100us) */
  for (timeout = 50; timeout != 0; timeout--) {
    udelay(1000); /* wait 1 ms */
    /* check that we got something on the data port */
    x = dsp_read(port);
    if (x < 0) continue; /* nothing available yet */
    if (x == 0xAA) return(0); /* 0xAA means 'OK' */
    return(-1); /* otherwise there's a problem */
  }
  /* if we are here, then a timeout occured */
  return(-1);
}


int dsp_read(unsigned short port) {
  /* When DSP data is available, it can be read in from the Read Data port.
   * Before the data is read in, bit-7 of the Read-Buffer Status port must be
   * checked to ensure that there is data to read. If bit-7 is 1, then there
   * is data to read. Otherwise, no data is available. */
  if ((inp(IO_RDSTAT) & 128) == 0) return(-1); /* nothing to read */
  return(inp(IO_READ));
}


void dsp_write(unsigned short port, int databyte) {
  /* DSP commands and data are sent through the Write Command/Data port.
   * Before data is written to the DSP, bit-7 of the Write-Buffer Status port
   * must be checked to ensure that the DSP command/data buffer is empty. If
   * bit-7 is 0, the DSP buffer is empty and is ready to receive commands or
   * data. Otherwise, no commands or data should be written to the DSP. */
  while ((inp(IO_WRSTAT) & 128) != 0); /* wait for the 'write' buffer to become available */
  outp(IO_WRITE, databyte);
}
