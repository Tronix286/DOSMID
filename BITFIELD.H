/*
 * This file is part of the DOSMid project.
 * http://dosmid.sourceforge.net
 *
 * Copyright (C) 2018 Mateusz Viste
 *
 * Provides definitions for bit field operations.
 */

#ifndef bitfield_h

/* set bit 'b' in bit array 'map' */
#define BIT_SET(map, b) ((unsigned char *)map)[(b) >> 3] |= (1 << ((b) & 7))

/* get bit 'b' from bit array 'map' (0 if not set, non-zero otherwise) */
#define BIT_GET(map, b) (((unsigned char *)map)[(b) >> 3] & (1 << ((b) & 7)))

#endif
