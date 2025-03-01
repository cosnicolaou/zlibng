#ifndef DEFLATE_H_
#define DEFLATE_H_
/* deflate.h -- internal compression state
 * Copyright (C) 1995-2016 Jean-loup Gailly
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* @(#) $Id$ */

#include "zutil.h"
#include "gzendian.h"

/* define NO_GZIP when compiling if you want to disable gzip header and
   trailer creation by deflate().  NO_GZIP would be used to avoid linking in
   the crc code when it is not needed.  For shared libraries, gzip encoding
   should be left enabled. */
#ifndef NO_GZIP
#  define GZIP
#endif

#define NIL 0
/* Tail of hash chains */


/* ===========================================================================
 * Internal compression state.
 */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define Buf_size 16
/* size of bit buffer in bi_buf */

#define END_BLOCK 256
/* end of block literal code */

#define INIT_STATE    42    /* zlib header -> BUSY_STATE */
#ifdef GZIP
#  define GZIP_STATE  57    /* gzip header -> BUSY_STATE | EXTRA_STATE */
#endif
#define EXTRA_STATE   69    /* gzip extra block -> NAME_STATE */
#define NAME_STATE    73    /* gzip file name -> COMMENT_STATE */
#define COMMENT_STATE 91    /* gzip comment -> HCRC_STATE */
#define HCRC_STATE   103    /* gzip header CRC -> BUSY_STATE */
#define BUSY_STATE   113    /* deflate -> FINISH_STATE */
#define FINISH_STATE 666    /* stream complete */
/* Stream status */


/* Data structure describing a single value and its code string. */
typedef struct ct_data_s {
    union {
        uint16_t  freq;       /* frequency count */
        uint16_t  code;       /* bit string */
    } fc;
    union {
        uint16_t  dad;        /* father node in Huffman tree */
        uint16_t  len;        /* length of bit string */
    } dl;
} ct_data;

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

typedef struct static_tree_desc_s  static_tree_desc;

typedef struct tree_desc_s {
    ct_data                *dyn_tree;  /* the dynamic tree */
    int                    max_code;   /* largest code with non zero frequency */
    const static_tree_desc *stat_desc; /* the corresponding static tree */
} tree_desc;

typedef uint16_t Pos;
typedef unsigned IPos;

/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

typedef struct internal_state {
    PREFIX3(stream)      *strm;            /* pointer back to this zlib stream */
    int                  status;           /* as the name implies */
    unsigned char        *pending_buf;     /* output still pending */
    unsigned long        pending_buf_size; /* size of pending_buf */
    unsigned char        *pending_out;     /* next pending byte to output to the stream */
    uint32_t             pending;          /* nb of bytes in the pending buffer */
    int                  wrap;             /* bit 0 true for zlib, bit 1 true for gzip */
    PREFIX(gz_headerp)   gzhead;           /* gzip header information to write */
    uint32_t             gzindex;          /* where in extra, name, or comment */
    unsigned char        method;           /* can only be DEFLATED */
    int                  last_flush;       /* value of flush param for previous deflate call */

#ifdef X86_PCLMULQDQ_CRC
    unsigned crc0[4 * 5];
#endif

                /* used by deflate.c: */

    unsigned int  w_size;            /* LZ77 window size (32K by default) */
    unsigned int  w_bits;            /* log2(w_size)  (8..16) */
    unsigned int  w_mask;            /* w_size - 1 */

    unsigned char *window;
    /* Sliding window. Input bytes are read into the second half of the window,
     * and move to the first half later to keep a dictionary of at least wSize
     * bytes. With this organization, matches are limited to a distance of
     * wSize-MAX_MATCH bytes, but this ensures that IO is always
     * performed with a length multiple of the block size. Also, it limits
     * the window size to 64K, which is quite useful on MSDOS.
     * To do: use the user input buffer as sliding window.
     */

    unsigned long window_size;
    /* Actual size of window: 2*wSize, except when the user input buffer
     * is directly used as sliding window.
     */

    Pos *prev;
    /* Link to older string with same hash index. To limit the size of this
     * array to 64K, this link is maintained only for the last 32K strings.
     * An index in this array is thus a window index modulo 32K.
     */

    Pos *head; /* Heads of the hash chains or NIL. */

    unsigned int  ins_h;             /* hash index of string to be inserted */
    unsigned int  hash_size;         /* number of elements in hash table */
    unsigned int  hash_bits;         /* log2(hash_size) */
    unsigned int  hash_mask;         /* hash_size-1 */

    #if !defined(__x86_64) && !defined(__i386_)
    unsigned int  hash_shift;
    #endif
    /* Number of bits by which ins_h must be shifted at each input
     * step. It must be such that after MIN_MATCH steps, the oldest
     * byte no longer takes part in the hash key, that is:
     *   hash_shift * MIN_MATCH >= hash_bits
     */

    long block_start;
    /* Window position at the beginning of the current output block. Gets
     * negative when the window is moved backwards.
     */

    unsigned int match_length;       /* length of best match */
    IPos         prev_match;         /* previous match */
    int          match_available;    /* set if previous match exists */
    unsigned int strstart;           /* start of string to insert */
    unsigned int match_start;        /* start of matching string */
    unsigned int lookahead;          /* number of valid bytes ahead in window */

    unsigned int prev_length;
    /* Length of the best match at previous step. Matches not greater than this
     * are discarded. This is used in the lazy match evaluation.
     */

    unsigned int max_chain_length;
    /* To speed up deflation, hash chains are never searched beyond this
     * length.  A higher limit improves compression ratio but degrades the
     * speed.
     */

    unsigned int max_lazy_match;
    /* Attempt to find a better match only when the current match is strictly
     * smaller than this value. This mechanism is used only for compression
     * levels >= 4.
     */
#   define max_insert_length  max_lazy_match
    /* Insert new strings in the hash table only if the match length is not
     * greater than this length. This saves time but degrades compression.
     * max_insert_length is used only for compression levels <= 3.
     */

    int level;    /* compression level (1..9) */
    int strategy; /* favor or force Huffman coding*/

    unsigned int good_match;
    /* Use a faster search when the previous match is longer than this */

    int nice_match; /* Stop searching when current match exceeds this */

                /* used by trees.c: */
    /* Didn't use ct_data typedef below to suppress compiler warning */
    struct ct_data_s dyn_ltree[HEAP_SIZE];   /* literal and length tree */
    struct ct_data_s dyn_dtree[2*D_CODES+1]; /* distance tree */
    struct ct_data_s bl_tree[2*BL_CODES+1];  /* Huffman tree for bit lengths */

    struct tree_desc_s l_desc;               /* desc. for literal tree */
    struct tree_desc_s d_desc;               /* desc. for distance tree */
    struct tree_desc_s bl_desc;              /* desc. for bit length tree */

    uint16_t bl_count[MAX_BITS+1];
    /* number of codes at each bit length for an optimal tree */

    int heap[2*L_CODES+1];      /* heap used to build the Huffman trees */
    int heap_len;               /* number of elements in the heap */
    int heap_max;               /* element of largest frequency */
    /* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
     * The same heap array is used to build all trees.
     */

    unsigned char depth[2*L_CODES+1];
    /* Depth of each subtree used as tie breaker for trees of equal frequency
     */

    unsigned char *sym_buf;       /* buffer for distances and literals/lengths */

    unsigned int  lit_bufsize;
    /* Size of match buffer for literals/lengths.  There are 4 reasons for
     * limiting lit_bufsize to 64K:
     *   - frequencies can be kept in 16 bit counters
     *   - if compression is not successful for the first block, all input
     *     data is still in the window so we can still emit a stored block even
     *     when input comes from standard input.  (This can also be done for
     *     all blocks if lit_bufsize is not greater than 32K.)
     *   - if compression is not successful for a file smaller than 64K, we can
     *     even emit a stored file instead of a stored block (saving 5 bytes).
     *     This is applicable only for zip (not gzip or zlib).
     *   - creating new Huffman trees less frequently may not provide fast
     *     adaptation to changes in the input data statistics. (Take for
     *     example a binary file with poorly compressible code followed by
     *     a highly compressible string table.) Smaller buffer sizes give
     *     fast adaptation but have of course the overhead of transmitting
     *     trees more frequently.
     *   - I can't count above 4
     */

    unsigned int sym_next;      /* running index in sym_buf */
    unsigned int sym_end;       /* symbol table full when sym_next reaches this */

    unsigned long opt_len;        /* bit length of current block with optimal trees */
    unsigned long static_len;     /* bit length of current block with static trees */
    unsigned int matches;         /* number of string matches in current block */
    unsigned int insert;          /* bytes at end of window left to insert */

#ifdef ZLIB_DEBUG
    unsigned long compressed_len; /* total bit length of compressed file mod 2^32 */
    unsigned long bits_sent;      /* bit length of compressed data sent mod 2^32 */
#endif

    uint16_t bi_buf;
    /* Output buffer. bits are inserted starting at the bottom (least
     * significant bits).
     */
    int bi_valid;
    /* Number of valid bits in bi_buf.  All bits above the last valid bit
     * are always zero.
     */

    unsigned long high_water;
    /* High water mark offset in window for initialized bytes -- bytes above
     * this are set to zero in order to avoid memory check warnings when
     * longest match routines access bytes past the input.  This is then
     * updated to the new high water mark.
     */
    int block_open;
    /* Whether or not a block is currently open for the QUICK deflation scheme.
     * This is set to 1 if there is an active block, or 0 if the block was just
     * closed.
     */

} deflate_state;

typedef enum {
    need_more,      /* block not completed, need more input or more output */
    block_done,     /* block flush performed */
    finish_started, /* finish started, need only more output at next deflate */
    finish_done     /* finish done, accept no more input or output */
} block_state;

/* Output a byte on the stream.
 * IN assertion: there is enough room in pending_buf.
 */
#define put_byte(s, c) {s->pending_buf[s->pending++] = (unsigned char)(c);}

/* ===========================================================================
 * Output a short LSB first on the stream.
 * IN assertion: there is enough room in pendingBuf.
 */
static inline void put_short(deflate_state *s, uint16_t w) {
#if BYTE_ORDER == BIG_ENDIAN
  w = ZSWAP16(w);
#endif
  MEMCPY(&(s->pending_buf[s->pending]), &w, sizeof(uint16_t));
  s->pending += 2;
}

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST(s)  ((s)->w_size-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

#define WIN_INIT MAX_MATCH
/* Number of bytes after end of data in window to initialize in order to avoid
   memory checker errors from longest match routines */


void ZLIB_INTERNAL zng_fill_window_c(deflate_state *s);

        /* in trees.c */
void ZLIB_INTERNAL _zng_tr_init(deflate_state *s);
int ZLIB_INTERNAL _zng_tr_tally(deflate_state *s, unsigned dist, unsigned lc);
void ZLIB_INTERNAL _zng_tr_flush_block(deflate_state *s, char *buf, unsigned long stored_len, int last);
void ZLIB_INTERNAL _zng_tr_flush_bits(deflate_state *s);
void ZLIB_INTERNAL _zng_tr_align(deflate_state *s);
void ZLIB_INTERNAL _zng_tr_stored_block(deflate_state *s, char *buf, unsigned long stored_len, int last);
void ZLIB_INTERNAL zng_bi_windup(deflate_state *s);

#define d_code(dist) ((dist) < 256 ? _zng_dist_code[dist] : _zng_dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. _zng_dist_code[256] and _zng_dist_code[257] are never
 * used.
 */

#ifndef ZLIB_DEBUG
/* Inline versions of _zng_tr_tally for speed: */

# if defined(GEN_TREES_H)
    extern unsigned char ZLIB_INTERNAL _zng_length_code[];
    extern unsigned char ZLIB_INTERNAL _zng_dist_code[];
# else
    extern const unsigned char ZLIB_INTERNAL _zng_length_code[];
    extern const unsigned char ZLIB_INTERNAL _zng_dist_code[];
# endif

# define _zng_tr_tally_lit(s, c, flush) \
  { unsigned char cc = (c); \
    s->sym_buf[s->sym_next++] = 0; \
    s->sym_buf[s->sym_next++] = 0; \
    s->sym_buf[s->sym_next++] = cc; \
    s->dyn_ltree[cc].Freq++; \
    flush = (s->sym_next == s->sym_end); \
  }
# define _zng_tr_tally_dist(s, distance, length, flush) \
  { unsigned char len = (unsigned char)(length); \
    uint16_t dist = (uint16_t)(distance); \
    s->sym_buf[s->sym_next++] = dist; \
    s->sym_buf[s->sym_next++] = dist >> 8; \
    s->sym_buf[s->sym_next++] = len; \
    dist--; \
    s->dyn_ltree[_zng_length_code[len]+LITERALS+1].Freq++; \
    s->dyn_dtree[d_code(dist)].Freq++; \
    flush = (s->sym_next == s->sym_end); \
  }
#else
#   define _zng_tr_tally_lit(s, c, flush) flush = _zng_tr_tally(s, 0, c)
#   define _zng_tr_tally_dist(s, distance, length, flush) \
              flush = _zng_tr_tally(s, distance, length)
#endif

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */

#ifdef NOT_TWEAK_COMPILER
#define TRIGGER_LEVEL 6
#else
#define TRIGGER_LEVEL 5
#endif

#if defined(__x86_64) || defined(__i386_)
#define UPDATE_HASH(s, h, i) \
    do {\
        if (s->level < TRIGGER_LEVEL) \
            h = (3483 * (s->window[i]) +\
                 23081* (s->window[i+1]) +\
                 6954 * (s->window[i+2]) +\
                 20947* (s->window[i+3])) & s->hash_mask;\
        else\
            h = (25881* (s->window[i]) +\
                 24674* (s->window[i+1]) +\
                 25811* (s->window[i+2])) & s->hash_mask;\
    } while (0)
#else
#   define UPDATE_HASH(s, h, i) (h = (((h) << s->hash_shift) ^ (s->window[i + (MIN_MATCH-1)])) & s->hash_mask)
#endif

#ifndef ZLIB_DEBUG
#  define send_code(s, c, tree) send_bits(s, tree[c].Code, tree[c].Len)
/* Send a code of the given tree. c and tree must not have side effects */

#else /* ZLIB_DEBUG */
#  define send_code(s, c, tree) \
    {  if (zng_z_verbose > 2) { \
           fprintf(stderr, "\ncd %3d ", (c)); \
       } \
       send_bits(s, tree[c].Code, tree[c].Len); \
     }
#endif

#ifdef ZLIB_DEBUG
void send_bits(deflate_state *s, int value, int length);
#else
#define send_bits(s, value, length) \
{ int len = length;\
  if (s->bi_valid > (int)Buf_size - len) {\
    int val = (int)value;\
    s->bi_buf |= (uint16_t)val << s->bi_valid;\
    put_short(s, s->bi_buf);\
    s->bi_buf = (uint16_t)val >> (Buf_size - s->bi_valid);\
    s->bi_valid += len - Buf_size;\
  } else {\
    s->bi_buf |= (uint16_t)(value) << s->bi_valid;\
    s->bi_valid += len;\
  }\
}
#endif

#endif /* DEFLATE_H_ */
