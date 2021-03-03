/*
 * Simple wrapper for writing to a RS232 port. This file is part of the
 * DOSMid project.
 *
 * Copyright (c) 2015, Mateusz Viste
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <conio.h> /* inp() */
#include <dos.h>   /* MK_FP() */

#include "rs232.h"

/* get the I/O port for COMx (1..4) */
unsigned short rs232_getport(int x) {
  unsigned short far *bioscomtable = MK_FP(0,0x400);
  unsigned short res;
  if (x < 1) return(0);
  if (x > 4) return(0);
  res = bioscomtable[x - 1];
  return(res);
}

/* check if the COM port is ready for write. loops for some time waiting.
 * returns 0 if port seems ready eventually, non-zero otherwise. can be used
 * to verify the rs232 presence */
int rs232_check(unsigned short port) {
  int i = 4096; /* cycles up to 4K times (should be enough for any UART) */
  while (i-- > 0) {
    if ((inp(port + 5) & 0x20) != 0) return(0);
  }
  /* the port wouldn't become ready - exit with error */
  return(-1);
}

/* write a byte to the COM port at 'port'. this function will block if the
 * UART is not ready to transmit yet. */
void rs232_write(unsigned short port, int data) {
  /* wait for the UART to become ready for write */
  while ((inp(port + 5) & 0x20) == 0);
  /* write the data byte now */
  outp(port, data);
}

/* read a byte from COM port at 'port'. returns the read byte, or -1 if
 * nothing was available to read. */
int rs232_read(unsigned short port) {
  /* if nothing awaits, return -1 */
  if ((inp(port + 5) & 0x01) == 0) return(-1);
  /* otherwise read from port and return the result */
  return(inp(port));
}
