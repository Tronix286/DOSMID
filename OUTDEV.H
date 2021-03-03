/*
 * Wrapper for outputing MIDI commands to different devices.
 *
 * Copyright (C) 2014-2018, Mateusz Viste
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

#ifndef outdev_h_sentinel
#define outdev_h_sentinel

enum outdev_types {
  DEV_NONE,
  DEV_MPU401,
  DEV_AWE,
  DEV_OPL,
  DEV_OPL2,
  DEV_OPL3,
  DEV_RS232,
  DEV_SBMIDI,
  DEV_GUS,
  DEV_CMS
};


/* inits the out device, also selects the out device, from one of these:
 *  DEV_MPU401
 *  DEV_AWE
 *  DEV_OPL
 *  DEV_RS232
 *  DEV_SBMIDI
 *  DEV_NONE
 *
 * This should be called only ONCE, when program starts.
 * Returns NULL on success, or a pointer to an error message otherwise.
 */
char *dev_init(enum outdev_types dev, unsigned short port, char *sbank);

/* pre-load a patch (so far needed only for GUS) */
void dev_preloadpatch(enum outdev_types dev, int p);

/* returns the device that has been inited/selected */
enum outdev_types dev_getcurdev(void);

/* close/deinitializes the out device */
void dev_close(void);

/* clears/reinits the out device (turns all sounds off...). this can be used
 * often (typically: between each song). */
void dev_clear(void);

/* activate note on channel */
void dev_noteon(int channel, int note, int velocity);

/* disable note on channel */
void dev_noteoff(int channel, int note);

/* adjust the pitch wheel on channel */
void dev_pitchwheel(int channel, int wheelvalue);

/* send a 'controller' message */
void dev_controller(int channel, int id, int val);

/* channel aftertouch */
void dev_chanpressure(int channel, int pressure);

/* key aftertouch */
void dev_keypressure(int channel, int note, int pressure);

/* should be called by the application from time to time */
void dev_tick(void);

/* sets a "program" (meaning an instrument) on a channel */
void dev_setprog(int channel, int program);

/* sends a raw sysex string to the device */
void dev_sysex(int channel, unsigned char *buff, int bufflen);

#endif
