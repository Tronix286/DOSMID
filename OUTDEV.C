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

#include <conio.h>  /* outp(), inp() */
#include <dos.h>    /* _disable(), _enable() */
#include <malloc.h> /* _fmalloc(), _ffree() */

#ifdef OPL
#include "opl.h"
#endif

#ifdef CMS
#include "cms.h"
#endif

#include "fio.h"
#include "gus.h"
#include "mpu401.h"
#include "rs232.h"
#include "sbdsp.h"

#ifdef SBAWE
#include "awe32/ctaweapi.h"
static char far *presetbuf = NULL; /* used to allocate presets for custom sound banks */
#endif

#include "outdev.h" /* include self for control */

/* force the compiler to load valid DS segment value before calling
 * the AWE32 API functions (in far data models, where DS is floating) */
#pragma aux __pascal "^" parm loadds reverse routine [] \
                         value struct float struct caller [] \
                         modify [ax bx cx dx es];


static enum outdev_types outdev = DEV_NONE;
static unsigned short outport = 0;


/* loads a SBK sound font to AWE hardware */
#ifdef SBAWE
static int awe_loadfont(char *filename) {
  struct fiofile_t f;
  SOUND_PACKET sp;
  long banks[1];
  int i;
  char buffer[PACKETSIZE];
  awe32TotalPatchRam(&sp); /* get available patch DRAM */
  if (sp.total_patch_ram < 512*1024) return(-1);
  /* Setup bank sizes with all available RAM */
  banks[0] = sp.total_patch_ram;
  sp.banksizes = banks;
  sp.total_banks = 1; /* total number of banks */
  if (awe32DefineBankSizes(&sp) != 0) return(-1);
  /* Open SoundFont Bank */
  if (fio_open(filename, FIO_OPEN_RD, &f) != 0) return(-1);
  fio_read(&f, buffer, PACKETSIZE); /* read sf header */
  /* prepare stuff */
  sp.bank_no = 0;
  sp.data = buffer;
  if (awe32SFontLoadRequest(&sp) != 0) {
    fio_close(&f);
    return(-1); /* invalid soundfont file */
  }
  /* stream sound samples into the hardware */
  if (sp.no_sample_packets > 0) {
    fio_seek(&f, FIO_SEEK_START, sp.sample_seek); /* move pointer to where instruments begin */
    for (i = 0; i < sp.no_sample_packets; i++) {
      if ((fio_read(&f, sp.data, PACKETSIZE) != PACKETSIZE) || (awe32StreamSample(&sp))) {
        fio_close(&f);
        return(-1);
      }
    }
  }
  /* load presets to memory */
  presetbuf = _fmalloc(sp.preset_read_size);
  if (presetbuf == NULL) { /* out of mem! */
    fio_close(&f);
    return(-1);
  }
  sp.presets = presetbuf;
  fio_seek(&f, FIO_SEEK_START, sp.preset_seek);
  fio_read(&f, sp.presets, sp.preset_read_size);
  /* close the sf file */
  fio_close(&f);
  /* apply presets to hardware */
  if (awe32SetPresets(&sp) != 0) {
    _ffree(presetbuf);
    presetbuf = NULL;
    return(-1);
  }
  return(0);
}
#endif


/* inits the out device, also selects the out device, from one of these:
 *  DEV_MPU401
 *  DEV_AWE
 *  DEV_OPL
 *  DEV_RS232
 *  DEV_SBMIDI
 *  DEV_GUS
 *  DEV_NONE
 *
 * This should be called only ONCE, when program starts.
 * Returns NULL on success, or a pointer to an error string otherwise. */
char *dev_init(enum outdev_types dev, unsigned short port, char *sbank) {
  outdev = dev;
  outport = port;
  switch (outdev) {
    case DEV_MPU401:
      /* reset the MPU401 */
      if (mpu401_rst(outport) != 0) return("MPU doesn't answer");
      /* put it into UART mode */
      mpu401_uart(outport);
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32NumG = 30; /* global var used by the AWE lib, must be set to 30 for
                         DRAM sound fonts to work properly */
      if (awe32Detect(outport) != 0) return("No EMU8000 chip detected");
      if (awe32InitHardware() != 0) return("EMU8000 initialization failed");
      /* preload GM samples from AWE's ROM */
      if (sbank == NULL) {
        awe32SoundPad.SPad1 = awe32SPad1Obj;
        awe32SoundPad.SPad2 = awe32SPad2Obj;
        awe32SoundPad.SPad3 = awe32SPad3Obj;
        awe32SoundPad.SPad4 = awe32SPad4Obj;
        awe32SoundPad.SPad5 = awe32SPad5Obj;
        awe32SoundPad.SPad6 = awe32SPad6Obj;
        awe32SoundPad.SPad7 = awe32SPad7Obj;
      } else if (awe_loadfont(sbank) != 0) {
        dev_close();
        return("Sound bank could not be loaded");
      }
      if (awe32InitMIDI() != 0) {
        dev_close();
        return("EMU8000 MIDI processor initialization failed");
      }
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsReset(outport);
#endif
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
    {
      int res;
      res = opl_init(outport);
      if (res < 0) return("No OPL2/OPL3 device detected");
      /* change the outdev device depending on OPL autodetection */
      if (res == 0) {
        outdev = DEV_OPL2;
      } else {
        outdev = DEV_OPL3;
      }
      /* load a custom sound bank, if any provided */
      if (sbank != NULL) {
        if (opl_loadbank(sbank) != 0) {
          dev_close();
          return("OPL sound bank could not be loaded");
        }
      }
    }
#endif
      break;
    case DEV_RS232:
      if (rs232_check(outport) != 0) return("RS232 failure");
      break;
    case DEV_SBMIDI:
      /* The DSP has to be reset before it is first programmed. The reset
       * causes it to perform an initialization and returns it to its default
       * state. The DSP reset is done through the Reset port. */
      if (dsp_reset(outport) != 0) return("SB DSP initialization failure");
      dsp_write(outport, 0x30); /* switch the MIDI I/O into polling mode */
      break;
    case DEV_GUS:
      gus_open(port);
      break;
    case DEV_NONE:
      break;
  }
  dev_clear();
  return(NULL);
}


/* pre-load a patch (so far needed only for GUS) */
void dev_preloadpatch(enum outdev_types dev, int p) {
  switch (dev) {
    case DEV_MPU401:
    case DEV_AWE:
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
    case DEV_RS232:
    case DEV_SBMIDI:
    case DEV_CMS:
      break;
    case DEV_GUS:
      gus_loadpatch(p);
      break;
    case DEV_NONE:
      break;
  }
}


/* returns the device that has been inited/selected */
enum outdev_types dev_getcurdev(void) {
  return(outdev);
}


/* close/deinitializes the out device */
void dev_close(void) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_rst(outport); /* resets it to intelligent mode */
      break;
    case DEV_AWE:
#ifdef SBAWE
      /* Creative recommends to disable interrupts during AWE shutdown */
      _disable();
      awe32Terminate();
      _enable();
      /* free memory used by custom sound banks */
      if (presetbuf != NULL) {
        _ffree(presetbuf);
        presetbuf = NULL;
      }
#endif
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_close(outport);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsReset(outport);
#endif
      break;
    case DEV_RS232:
      break;
    case DEV_SBMIDI:
      /* To terminate UART mode, send a DSP reset command. The reset command
       * behaves differently while the DSP is in MIDI UART mode. It terminates
       * MIDI UART mode and restores all the DSP parameters to the states
       * prior to entering MIDI UART mode. If your application was run in MIDI
       * UART mode, it important that you send the DSP reset command to exit
       * the MIDI UART mode when your application terminates. */
      dsp_reset(outport); /* I don't use MIDI UART mode because it requires */
                          /* DSP v2.x, but reseting the chip seems like a   */
      break;              /* good thing to do anyway                        */
    case DEV_GUS:
      gus_close();
      break;
    case DEV_NONE:
      break;
  }
}


/* clears/reinits the out device (turns all sounds off...). this can be used
 * often (typically: between each song) */
void dev_clear(void) {
  int i;
  /* iterate on MIDI channels and send 'off' messages */
  for (i = 0; i < 16; i++) {
    dev_controller(i, 123, 0);   /* "all notes off" */
    dev_controller(i, 120, 0);   /* "all sounds off" */
    dev_controller(i, 121, 0);   /* "all controllers off" */
  }
  /* execute hardware-specific actions */
  switch (outdev) {
    case DEV_MPU401:
    case DEV_AWE:
    case DEV_RS232:
    case DEV_SBMIDI:
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_clear(outport);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsReset(outport);
#endif
      break;
    case DEV_GUS:
      gus_allnotesoff();
      gus_unloadpatches();
      break;
    case DEV_NONE:
      break;
  }
  /* reset the device's master volume via sysex */
  dev_sysex(0x7F, "\xF0\x7F\x7F\x04\x01\x7F\x7F\xF7", 8);
}


/* activate note on channel */
void dev_noteon(int channel, int note, int velocity) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x90 | channel);  /* Send note ON to selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn ON */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, velocity);        /* Send velocity */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_noteon(outport, channel, note, velocity);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32NoteOn(channel, note, velocity);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsNoteOn(channel, note, velocity);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0x90 | channel); /* Send note ON to selected channel */
      rs232_write(outport, note);           /* Send note number to turn ON */
      rs232_write(outport, velocity);       /* Send velocity */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0x90 | channel);   /* Send note ON to selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, note);             /* Send note number to turn ON */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, velocity);         /* Send velocity */
      break;
    case DEV_GUS:
      gus_write(0x90 | channel);            /* Send note ON to selected channel */
      gus_write(note);                      /* Send note number to turn ON */
      gus_write(velocity);                  /* Send velocity */
      break;
    case DEV_NONE:
      break;
  }
}


/* disable note on channel */
void dev_noteoff(int channel, int note) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x80 | channel);  /* Send note OFF code on selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn OFF */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 64);              /* Send velocity */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_noteoff(outport, channel, note);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32NoteOff(channel, note, 64);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsNoteOff(channel, note);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0x80 | channel); /* 'note off' + channel selector */
      rs232_write(outport, note);           /* note number */
      rs232_write(outport, 64);             /* velocity */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0x80 | channel); /* Send note OFF code on selected channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, note);           /* Send note number to turn OFF */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 64);             /* Send velocity */
      break;
    case DEV_GUS:
      gus_write(0x80 | channel);   /* 'note off' + channel selector */
      gus_write(note);             /* note number */
      gus_write(64);               /* velocity */
      break;
    case DEV_NONE:
      break;
  }
}


/* adjust the pitch wheel of a channel */
void dev_pitchwheel(int channel, int wheelvalue) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xE0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue & 127);/* Send the lowest (least significant) 7 bits of the wheel value */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue >> 7); /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_pitchwheel(outport, channel, wheelvalue);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cms_pitchwheel(outport, channel, wheelvalue);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32PitchBend(channel, wheelvalue & 127, wheelvalue >> 7);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xE0 | channel);   /* Send selected channel */
      rs232_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      rs232_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0xE0 | channel);   /* Send selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_GUS:
      gus_write(0xE0 | channel);   /* Send selected channel */
      gus_write(wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      gus_write(wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_NONE:
      break;
  }
}


/* send a 'controller' message */
void dev_controller(int channel, int id, int val) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xB0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, id);              /* Send the controller's id */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, val);             /* Send controller's value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_controller(outport, channel, id, val);
#endif
      break;
    case DEV_CMS:
#ifdef CMS
      cmsController(channel, id, val);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32Controller(channel, id, val);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xB0 | channel);  /* Send selected channel */
      rs232_write(outport, id);              /* Send the controller's id */
      rs232_write(outport, val);             /* Send controller's value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xB0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, id);              /* Send the controller's id */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, val);             /* Send controller's value */
      break;
    case DEV_GUS:
      gus_write(0xB0 | channel);           /* Send selected channel */
      gus_write(id);                       /* Send the controller's id */
      gus_write(val);                      /* Send controller's value */
      break;
    case DEV_NONE:
      break;
  }
}


void dev_chanpressure(int channel, int pressure) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xD0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* nothing to do */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32ChannelPressure(channel, pressure);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xD0 | channel);  /* Send selected channel */
      rs232_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xD0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_GUS:
      gus_write(0xD0 | channel);           /* Send selected channel */
      gus_write(pressure);                 /* Send the pressure value */
      break;
    case DEV_NONE:
      break;
  }
}


void dev_keypressure(int channel, int note, int pressure) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xA0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send the note we target */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* nothing to do */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32PolyKeyPressure(channel, note, pressure);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xA0 | channel);  /* Send selected channel */
      rs232_write(outport, note);            /* Send the note we target */
      rs232_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xA0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, note);            /* Send the note we target */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_GUS:
      gus_write(0xA0 | channel);           /* Send selected channel */
      gus_write(note);                     /* Send the note we target */
      gus_write(pressure);                 /* Send the pressure value */
      break;
    case DEV_NONE:
      break;
  }
}


/* should be called by the application from time to time */
void dev_tick(void) {
  switch (outdev) {
    case DEV_CMS:
#ifdef CMS
      cmsTick();
#endif
      break;
    case DEV_MPU401:
      mpu401_flush(outport);
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      break;
    case DEV_AWE:
      break;
    case DEV_RS232:
      /* I do nothing here - although flushing any incoming bytes would seem
       * to be the 'sane thing to do', it can lead sometimes to freezes on
       * systems where the RS232 UART always reports a 'read ready' status.
       * NOT flushing the UART, on the other hand, doesn't seem to affect
       * anything. */
      /* while (rs232_read(outport) >= 0); */
      break;
    case DEV_SBMIDI:
      break;
    case DEV_GUS:
      break;
    case DEV_NONE:
      break;
  }
}


/* sets a "program" (meaning an instrument) on a channel */
void dev_setprog(int channel, int program) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, 0xC0 | channel); /* Send channel */
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, program);        /* Send patch id */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_changeprog(channel, program);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32ProgramChange(channel, program);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xC0 | channel); /* Send channel */
      rs232_write(outport, program);        /* Send patch id */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0xC0 | channel); /* Send channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, program);        /* Send patch id */
      break;
    case DEV_GUS:
      /* NOTE I might (?) want to call gus_loadpatch() here */
      /*if (channel == 9) program |= 128;
      gus_loadpatch(program);*/
      gus_write(0xC0 | channel);          /* Send channel */
      gus_write(program);                 /* Send patch id */
      break;
    case DEV_NONE:
      break;
  }
}


/* sends a raw sysex string to the device */
void dev_sysex(int channel, unsigned char *buff, int bufflen) {
  int x;
  switch (outdev) {
    case DEV_MPU401:
      for (x = 0; x < bufflen; x++) {
        mpu401_waitwrite(outport);     /* Wait for port ready */
        outp(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* SYSEX is unsupported on OPL output */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32Sysex(channel, (unsigned char far *)buff, bufflen);
#endif
      break;
    case DEV_RS232:
      for (x = 0; x < bufflen; x++) {
        rs232_write(outport, buff[x]);      /* Send sysex data byte */
      }
      break;
    case DEV_SBMIDI:
      for (x = 0; x < bufflen; x++) {
        dsp_write(outport, 0x38);           /* MIDI output */
        dsp_write(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
    case DEV_GUS:
      for (x = 0; x < bufflen; x++) {
        gus_write(buff[x]);                 /* Send sysex data byte */
      }
      break;
    case DEV_NONE:
      break;
  }
}
