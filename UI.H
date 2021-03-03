/*
 * User interface routines of DOSMid.
 * Copyright (C) Mateusz Viste
 */

#ifndef ui_h_sentinel
#define ui_h_sentinel

#define UI_REFRESH_TUI        1
#define UI_REFRESH_NOTES      2
#define UI_REFRESH_TEMPO      4
#define UI_REFRESH_TITLECOPYR 8
#define UI_REFRESH_PROGS     16
#define UI_REFRESH_TIME      32
#define UI_REFRESH_VOLUME    64
#define UI_REFRESH_FNAME    128
#define UI_REFRESH_ALL     0xff

#define UI_TITLEMAXLEN 48
#define UI_TITLENODES 12
#define UI_FILENAMEMAXLEN 16

enum fileformats {
  FORMAT_UNKNOWN,
  FORMAT_MIDI,
  FORMAT_RMID,
  FORMAT_MUS
};

struct trackinfodata {
  unsigned long tempo;
  unsigned long totlen; /* total length, in seconds */
  unsigned long elapsedsec; /* time elapsed (in seconds) */
  unsigned short notestates[128]; /* here I record the state of every note on every channel, to turn all notes OFF in case of program termination */
  unsigned short channelsusage;  /* a bit field indicating what channels are used */
  unsigned char reqpatches[32]; /* bit field of 256 bits indicating what patches (programs) are used, 0-127=melodic ; 128-255=percussion */
  unsigned short miditimeunitdiv;
  unsigned char chanprogs[16];
  int titlescount;
  enum fileformats fileformat;
  int midiformat;
  unsigned short trackscount; /* number of tracks that were found in the file */
  char title[UI_TITLENODES][UI_TITLEMAXLEN];
  char filename[UI_FILENAMEMAXLEN];
};

/* inits ui, sets colorflag (0=mono ; non-zero=color) */
void ui_init(void);

/* cleanup and restores initial video mode */
void ui_close(void);

/* hides the blinking cursor */
void ui_hidecursor(void);

/* outputs an error message onscreen (title can be NULL) */
void ui_puterrmsg(char *title, char *errmsg);

/* draws the UI screen */
void ui_draw(struct trackinfodata *trackinfo, unsigned short *refreshflags, unsigned short *refreshchans, char *devname, unsigned int mpuport, int volume);

/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int getkey(void);

/* poll the keyboard, and return the next input key if any, or -1 */
int getkey_ifany(void);

#endif
