/*
 * XMS driver for DOSMid
 *
 * Copyright (C) 2014-2018 Mateusz Viste
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

#ifndef xms_h_sentinel
#define xms_h_sentinel

struct xms_struct {
  unsigned short handle;
  long memsize; /* allocated memory size, in bytes */
};

/* checks if a XMS driver is installed, inits it and allocates a memory block of memsize K-bytes.
 * if memsize is 0, then the maximum possible block will be allocated.
 * returns the amount of allocated memory (in K-bytes) on success, 0 otherwise. */
unsigned int xms_init(struct xms_struct *xms, unsigned short memsize);

/* free XMS memory */
void xms_close(struct xms_struct *xms);

/* copies a chunk of memory from conventional memory into the XMS block.
   returns 0 on sucess, non-zero otherwise. */
int xms_push(struct xms_struct *xms, void far *src, unsigned short len, long xmsoffset);

/* copies a chunk of memory from the XMS block into conventional memory */
int xms_pull(struct xms_struct *xms, long xmsoffset, void far *dst, unsigned short len);

#endif
