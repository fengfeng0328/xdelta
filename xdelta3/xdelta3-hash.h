/* xdelta 3 - delta compression tools and library
 * Copyright (C) 2001, 2003, 2004, 2005, 2006, 2007.  Joshua P. MacDonald
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _XDELTA3_HASH_H_
#define _XDELTA3_HASH_H_

#include "xdelta3-internal.h"

#if XD3_DEBUG
#define SMALL_HASH_DEBUG1(s,inp)                                  \
  uint32_t debug_state;                                           \
  uint32_t debug_hval = xd3_checksum_hash (& (s)->small_hash,     \
              xd3_scksum (&debug_state, (inp), (s)->smatcher.small_look))
#define SMALL_HASH_DEBUG2(s,inp)                                  \
  XD3_ASSERT (debug_hval == xd3_checksum_hash (& (s)->small_hash, \
              xd3_scksum (&debug_state, (inp), (s)->smatcher.small_look)))
#else
#define SMALL_HASH_DEBUG1(s,inp)
#define SMALL_HASH_DEBUG2(s,inp)
#endif /* XD3_DEBUG */

/* This is a good hash multiplier for 32-bit LCGs: see "linear
 * congruential generators of different sizes and good lattice
 * structure" */
static const uint32_t hash_multiplier = 1597334677U;

/***********************************************************************
 Permute stuff
 ***********************************************************************/

/* Update the checksum state. */
#if ADLER_LARGE_CKSUM
inline uint32_t
xd3_large_cksum_update (uint32_t cksum,
			const uint8_t *base,
			usize_t look) {
  uint32_t old_c = PERMUTE(base[0]);
  uint32_t new_c = PERMUTE(base[look]);
  uint32_t low   = ((cksum & 0xffff) - old_c + new_c) & 0xffff;
  uint32_t high  = ((cksum >> 16) - (old_c * look) + low) & 0xffff;
  return (high << 16) | low;
}
#else
/* TODO: revisit this topic */
#endif

#if UNALIGNED_OK
#define UNALIGNED_READ32(dest,src) (*(dest)) = (*(uint32_t*)(src))
#else
#define UNALIGNED_READ32(dest,src) memcpy((dest), (src), 4);
#endif

/* TODO: small cksum is hard-coded for 4 bytes (i.e., "look" is unused) */
static inline uint32_t
xd3_scksum (uint32_t *state,
            const uint8_t *base,
            const usize_t look)
{
  UNALIGNED_READ32(state, base);
  return (*state) * hash_multiplier;
}
static inline uint32_t
xd3_small_cksum_update (uint32_t *state,
			const uint8_t *base,
			usize_t look)
{
  UNALIGNED_READ32(state, base+1);
  return (*state) * hash_multiplier;
}

/***********************************************************************
 Ctable stuff
 ***********************************************************************/

static inline usize_t
xd3_checksum_hash (const xd3_hash_cfg *cfg, const usize_t cksum)
{
  return (cksum >> cfg->shift) ^ (cksum & cfg->mask);
}

/***********************************************************************
 Cksum function
 ***********************************************************************/

#if ADLER_LARGE_CKSUM
inline usize_t
xd3_large_cksum (const uint8_t *seg, const usize_t ln)
{
  usize_t i = 0;
  uint32_t low  = 0;
  uint32_t high = 0;

  for (; i < ln; i += 1)
    {
      low  += PERMUTE(*seg++);
      high += low;
    }

  return ((high & 0xffff) << 16) | (low & 0xffff);
}
#else
/* TODO */
#endif

#if XD3_ENCODER
static usize_t
xd3_size_hashtable_bits (usize_t slots)
{
  usize_t bits = (SIZEOF_USIZE_T * 8) - 1;
  usize_t i;

  for (i = 3; i <= bits; i += 1)
    {
      if (slots < (1U << i))
	{
	  /* TODO: this is compaction=1 in checksum_test.cc and maybe should
	   * not be fixed at -1. */
	  bits = i - 1; 
	  break;
	}
    }

  return bits;
}

static void
xd3_size_hashtable (xd3_stream    *stream,
		    usize_t        slots,
		    xd3_hash_cfg  *cfg)
{
  usize_t bits = xd3_size_hashtable_bits (slots);

  cfg->size  = (1 << bits);
  cfg->mask  = (cfg->size - 1);
  cfg->shift = (SIZEOF_USIZE_T * 8) - bits;
}
#endif

#endif
