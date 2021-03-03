/*
 * MUS loader for DOSMid
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

#include <string.h> /* memset() */

#include "bitfield.h"
#include "fio.h"
#include "mem.h"
#include "midi.h"

#include "mus.h" /* include self for control */

#define TICKLEN 500000l /* length of a single MUS tick, in us */
#define TIMEUNITDIV 70  /* the MIDI time unit divisor */


/* loads a MUS file into memory, returns the id of the first event on success,
 * or -1 on error. channelsusage contains 16 flags indicating what channels
 * are used. */
long mus_load(struct fiofile_t *f, unsigned long *totlen, unsigned short *timeunitdiv, unsigned short *channelsusage, void *reqpatches) {
  unsigned char hdr_or_chanvol[16];
  unsigned short scorestart;
  unsigned char bytebuff, bytebuff2, loadflag = 0;
  unsigned long event_dtime, nextwait = 0;
  unsigned short event_type;
  unsigned short event_channel;
  long res = -1;
  int tickduration = 0;
  long mslen = 0;
  struct midi_event_t midievent;

  /* read the 16 bytes header first, and populate hdr data */
  if (fio_read(f, hdr_or_chanvol, 16) != 16) return(-8);
  if ((hdr_or_chanvol[0] != 'M') || (hdr_or_chanvol[1] != 'U') || (hdr_or_chanvol[2] != 'S') || (hdr_or_chanvol[3] != 0x1A)) {
    return(-9);
  }
  scorestart = hdr_or_chanvol[6] | (hdr_or_chanvol[7] << 8);
  /* position the next reading position to first event */
  fio_seek(f, FIO_SEEK_START, scorestart);
  /* set tempo to 140 bpm (428571 us per quarter note) */
  memset(&midievent, 0, sizeof(struct midi_event_t));
  midievent.type = EVENT_TEMPO;
  *timeunitdiv = TIMEUNITDIV;
  midievent.data.tempoval = TICKLEN;
  if (pusheventqueue(&midievent, &res) != 0) return(MUS_OUTOFMEM);

  /* since now on, hdr_or_chanvol is used to store volume of channels */
  memset(hdr_or_chanvol, 0, 16);

  *channelsusage = 0; /* zero out the used instruments map */

  /* read events from the MUS file and translate them into midi events */
  for (;;) {
    if (fio_read(f, &bytebuff, 1) != 1) return(-5); /* if EOF, abort with error */
    event_channel = bytebuff & 0x0F;
    bytebuff >>= 4;
    event_type = bytebuff & 7;
    bytebuff >>= 3;
    event_dtime = bytebuff; /* if the 'last' bit is set, remember to read time after the event later */
    /* if channel is 15, it is percussion, and must be remapped to MIDI #9 */
    if (event_channel == 15) {
      event_channel = 9;
    } else if (event_channel == 9) {
      event_channel = 15;
    }
    /* clear out midievent to make room for the incoming MIDI message */
    memset(&midievent, 0, sizeof(struct midi_event_t));
    /* read complementary data, if any */
    switch (event_type) {
      case 0: /* release note (1 byte follows) */
        fio_read(f, &bytebuff, 1);
        if ((bytebuff & 128) != 0) return(-13); /* MSB should always be zero */
        midievent.type = EVENT_NOTEOFF;
        midievent.data.note.note = bytebuff;
        midievent.data.note.chan = event_channel;
        midievent.data.note.velocity = 0;
        break;
      case 1: /* play note (1 or 2 bytes follow) */
        *channelsusage |= (1 << event_channel); /* update the channel usage flags */
        fio_read(f, &bytebuff, 1);
        midievent.type = EVENT_NOTEON;
        midievent.data.note.note = bytebuff & 127;
        midievent.data.note.chan = event_channel;
        if ((bytebuff & 128) != 0) {
          fio_read(f, &(midievent.data.note.velocity), 1);
          hdr_or_chanvol[event_channel] = midievent.data.note.velocity; /* remember the last velocity, might be needed later */
        } else {
          midievent.data.note.velocity = hdr_or_chanvol[event_channel];
        }
        /* if percussion, note the associated instrument (program) as 'used' */
        if (event_channel == 9) BIT_SET(reqpatches, bytebuff | 128);
        break;
      case 2: /* pitch wheel (1 byte follows) */
        /* MIDI says that pitch wheel is a 14bit value with the center being
         * at 0x2000. MUS on the other hand provides only 8bits, so some
         * adjustement must be made as to fit into this scheme:
         *   0  =  two half-tones down
         *  64  =  one half-tone down
         * 128  =  normal (default)
         * 192  =  one half-tone up
         * 255  =  two half-tones up  */
        fio_read(f, &bytebuff, 1);
        midievent.type = EVENT_PITCH;
        midievent.data.pitch.wheel = bytebuff;
        midievent.data.pitch.wheel <<= 6; /* convert wheel value to 14 bits as expected by MIDI */
        midievent.data.pitch.chan = event_channel;
        break;
      case 3: /* sysex (1 byte follows) - I ignore SYSEX messages */
        fio_read(f, &bytebuff, 1);
        if ((bytebuff & 128) != 0) return(-11); /* MSB should always be zero */
        break;
      case 4: /* control (2 bytes follow) */
        fio_read(f, &bytebuff, 1);
        fio_read(f, &bytebuff2, 1);
        if ((bytebuff == 0) && (bytebuff2 < 128)) { /* change program */
          midievent.type = EVENT_PROGCHAN;
          midievent.data.prog.prog = bytebuff2;
          midievent.data.note.chan = event_channel;
          BIT_SET(reqpatches, bytebuff2);
        } else if ((bytebuff >= 1) && (bytebuff <= 9)) { /* else it maps directly to a MIDI controller message */
          int tmpmap[10] = {0,0,1,7,10,11,91,93,64,67}; /* map MUS controllers to MIDI controller IDs */
          midievent.type = EVENT_CONTROL;
          midievent.data.control.chan = event_channel;
          midievent.data.control.id = tmpmap[bytebuff];
          midievent.data.control.val = bytebuff2;
        } else { /* else it's an illegal byte pattern - abort mission, captain! */
          return(-91);
        }
        break;
      case 6: /* end of song (no byte follow) */
        if (pusheventqueue(NULL, NULL) != 0) return(MUS_OUTOFMEM);
        loadflag = 1;
        break;
      default: /* unknown event type - abort */
        return(-3);
        break;
    }
    /* if file loaded fine, break out of the loop now */
    if (loadflag != 0) break;
    /* fill in the delta time */
    midievent.deltatime = nextwait;
    /* if dtime is non-zero, read the number of ticks to wait before next note */
    nextwait = 0;
    while (event_dtime != 0) {
      if (fio_read(f, &bytebuff, 1) != 1) return(-6);
      event_dtime = bytebuff & 128;
      nextwait <<= 7;
      nextwait |= (bytebuff & 127);
    }
    /* push the event into memory */
    if (pusheventqueue(&midievent, NULL) != 0) return(MUS_OUTOFMEM);
    /* recompute total song's length */
    tickduration += nextwait;
    while (tickduration >= TIMEUNITDIV) {
      mslen += TICKLEN / 1000; /* mslen is in miliseconds, while TICKLEN is in us */
      tickduration -= TIMEUNITDIV;
    }
  }
  mslen += (tickduration / TIMEUNITDIV * TICKLEN) / 1000;
  *totlen = mslen / 1000; /* totlen is in seconds */
  /* */
  return(res);
}
