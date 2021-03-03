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

#include <dos.h> /* REGS */

#include "xms.h" /* include self for control */

/* a function pointer to save the XMS driver address */
void far (*xmsdrv)(void);

/* descriptor for XMS moves */
struct xms_move {
  long count;               /* number of bytes to move */
  unsigned short srchandle; /* source handle (0 for real memory) */
  long srcoffset;           /* source offset (or far pointer) */
  unsigned short dsthandle; /* destination handle (0 for real memory) */
  long dstoffset;           /* destination offset (or far pointer) */
};


/* used by xms_push() and xms_pull() to call the xms driver for data copy.
   returns 0 on success, non-zero otherwise */
static int xms_move(struct xms_move far *xmove) {
  unsigned short resax = 0;
  unsigned char resbl = 0;
  __asm {
    push bp                  ; save BP
    push ds                  ; save DS
    push si                  ; save SI
    mov ah, 0x0b             ; xms function is 0x0b
    lds si, [xmove]          ;
    call dword ptr [xmsdrv]  ; call the XMS driver
    pop si                   ; restore original SI
    pop ds                   ; restore original DS
    pop bp                   ; restore original BP
    mov resax, ax            ; save exit status
    mov resbl, bl            ; save result
  }
  /* check result */
  if (resax == 1) return(0);  /* AX = 0001h if the move is successful, 0000h otherwise */
  return(resbl);
}


/* returns the largest available block of free XMS memory */
static unsigned int xms_memfree(void) {
  unsigned short res = 0;
  __asm {
    mov ah, 0x08
    call dword ptr [xmsdrv]  ; call the XMS driver
    mov res, ax              ; AX = Size of the largest free block in K-bytes
  }
  return(res);
}


/* checks if a XMS driver is installed, inits it and allocates a memory block of memsize K-bytes.
 * if memsize is 0, then the maximum possible block will be allocated.
 * returns the amount of allocated memory (in K-bytes) on success, 0 otherwise. */
unsigned int xms_init(struct xms_struct *xms, unsigned short memsize) {
  union REGS regs;
  struct SREGS sregs;
  unsigned short axres = 0, dxres = 0;
  unsigned short freemem;
  /* check that an XMS driver is present */
  regs.x.ax = 0x4300;
  int86(0x2F, &regs, &regs);
  if (regs.h.al != 0x80) return(0);
  /* fetch the driver's API address */
  regs.x.ax = 0x4310;
  int86x(0x2F, &regs, &regs, &sregs);
  xmsdrv = (void far (*)()) MK_FP(sregs.es, regs.x.bx);
  xms->handle = 0;
  xms->memsize = 0;
  /* if memsize is 0, allocate as much as we can */
  freemem = xms_memfree();
  if ((memsize == 0) || (freemem < memsize)) memsize = freemem;
  if (memsize == 0) return(0);
  /* allocate the memory block */
  __asm {
    mov ah, 0x09
    mov dx, memsize
    call dword ptr [xmsdrv]  ; call the XMS driver
    mov dxres, dx
    mov axres, ax
  }
  xms->handle = dxres;
  if (axres != 1) return(0); /* if allocation failed... */
  xms->memsize = memsize;
  xms->memsize <<= 10; /* memsize is in kbytes, but we want to have it in bytes now */
  return(memsize);
}


/* free XMS memory */
void xms_close(struct xms_struct *xms) {
  unsigned short handle;
  handle = xms->handle;
  __asm {
    mov ah, 0x0a
    push dx                   ; save DX
    mov dx, handle            ; DX = Handle to the block to be freed
    call dword ptr [xmsdrv]   ; call the XMS driver
    pop dx;                   ; restore original DX
  }
}


/* copies a chunk of memory from conventional memory into the XMS block.
   returns 0 on sucess, non-zero otherwise. */
int xms_push(struct xms_struct *xms, void far *src, unsigned short len, long xmsoffset) {
  int res;
  struct xms_move xmove, far *ptr;
  ptr = &xmove;
  /* prepare the xms move struct */
  xmove.count = len;
  xmove.srchandle = 0; /* handle == 0 means 'see in conv. memory' */
  xmove.srcoffset = (unsigned long)src;
  xmove.dsthandle = xms->handle;
  xmove.dstoffset = xmsoffset;
  /* call the xms api */
  res = xms_move(ptr);
  return(res);
}


/* copies a chunk of memory from the XMS block into conventional memory.
   returns 0 on success, non-zero otherwise. */
int xms_pull(struct xms_struct *xms, long xmsoffset, void far *dst, unsigned short len) {
  int res;
  struct xms_move xmove, far *ptr;
  ptr = &xmove;
  /* prepare the xms move struct */
  xmove.count = len;
  xmove.srchandle = xms->handle;
  xmove.srcoffset = xmsoffset;
  xmove.dsthandle = 0; /* handle == 0 means 'see in conv. memory' */
  xmove.dstoffset = (unsigned long)dst;
  /* call the xms api */
  res = xms_move(ptr);
  return(res);
}
