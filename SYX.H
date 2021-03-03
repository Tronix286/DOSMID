/*
 * Reads sysex data from a SYX file, and returns a sysex binary string
 * This file is part of the DOSMid project. Copyright (C) Mateusz Viste.
 */

#ifndef SYX_H_SENTINEL

#define SYXERR_INVALIDPARAM  -1
#define SYXERR_UNEXPECTEDEOF -2
#define SYXERR_BUFFEROVERRUN -3
#define SYXERR_INVALIDHEADER -4
#define SYXERR_INVALIDFORMAT -5

int syx_fetchnext(struct fiofile_t *fh, unsigned char *buff, int bufflen);

#endif
