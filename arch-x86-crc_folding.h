/* arch-x86-crc_folding.h
 *
 * Compute the CRC32 using a parallelized folding approach with the PCLMULQDQ
 * instruction.
 *
 * Copyright (C) 2013 Intel Corporation Jim Kukunas
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef CRC_FOLDING_H_
#define CRC_FOLDING_H_

#include "deflate.h"

ZLIB_INTERNAL void zng_crc_fold_init(deflate_state *const);
ZLIB_INTERNAL uint32_t zng_crc_fold_512to32(deflate_state *const);
ZLIB_INTERNAL void zng_crc_fold_copy(deflate_state *const, unsigned char *, const unsigned char *, long);

#endif
