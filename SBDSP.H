/*
 * SoundBlaster DSP code
 * This file is part of the DOSMid project
 * Copyright (C) Mateusz Viste 2015
 */

#ifndef sbdsp_h_sentinel
#define sbdsp_h_sentinel

  /* Resets the DSP - should be used to initialise the SoundBlaster card.
   * Returns 0 on success, non-zero otherwise (meaning "no sb card found"). */
  int dsp_reset(unsigned short port);

  /* Reads from the DSP - non-blocking. Returns the databyte read, if any,
   * or a negative value if nothing available to read. */
  int dsp_read(unsigned short port);

  /* Writes to the DSP. This will block until the sb accepts the databyte. */
  void dsp_write(unsigned short port, int databyte);

#endif
