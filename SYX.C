/*
 * Reads sysex data from a SYX file, and returns a sysex binary string
 * This file is part of the DOSMid project. Copyright (C) Mateusz Viste.
 */

#include "fio.h"
#include "syx.h" /* include self for control */


/* fetch the next sysex event from file *fd, and copies it into *buff, up to
 * bufflen bytes. returns the length of the sysex string on success, 0 on
 * end of file, and a negative value on error. */
int syx_fetchnext(struct fiofile_t *fh, unsigned char *buff, int bufflen) {
  int reslen = 0;
  unsigned char bytebuff;
  /* quit immediately if any of the arguments is invalid */
  if ((fh == 0L) || (buff == 0L) || (bufflen < 1)) return(SYXERR_INVALIDPARAM);
  /* */
  for (;;) {
    if (fio_read(fh, &bytebuff, 1) != 1) { /* check for eof or i/o error */
      if (reslen == 0) return(0); /* clean end of file */
      return(SYXERR_UNEXPECTEDEOF); /* error - I expect sysex data to be terminated with F0 */
    }
    /* check for buffer overrun */
    if (reslen >= bufflen) return(SYXERR_BUFFEROVERRUN);
    /* if first byte, then it MUST be 0xF0 */
    if ((reslen == 0) && (bytebuff != 0xF0)) return(SYXERR_INVALIDHEADER);
    /* was it an invalid char? */
    if ((reslen != 0) && ((bytebuff & 128) != 0) && (bytebuff != 0xF7)) return(SYXERR_INVALIDFORMAT);
    /* copy the byte to buff */
    buff[reslen++] = bytebuff;
    /* was it the sysex terminator? */
    if (bytebuff == 0xF7) return(reslen);
  }
}
