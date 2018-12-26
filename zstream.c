#include "./zstream.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "./zlib-ng.h"

int zs_inflate_init(char* stream) {
  zng_stream* zs = (zng_stream*)stream;
  memset(zs, 0, sizeof(*zs));
  // 16 makes it understand only gzip files
  return zng_inflateInit2(zs, 16+15);
}

void zs_inflate_end(char* stream) { zng_inflateEnd((zng_stream*)stream); }

int zs_get_errno() { return errno; }

int zs_inflate_with_input(char* stream, void* in, int in_bytes, void* out,
                          int* out_bytes, int* consumed_input) {
  zng_stream* zs = (zng_stream*)stream;
  if (zs->avail_in != 0 || in_bytes <= 0) {
    abort();
  }
  zs->avail_in = in_bytes;
  zs->next_in = in;
  zs->next_out = out;
  zs->avail_out = *out_bytes;
  int ret = zng_inflate((zng_stream*)stream, Z_NO_FLUSH);
  if (ret == Z_OK || ret == Z_STREAM_END) {
    *out_bytes = zs->avail_out;
  }
  *consumed_input = (zs->avail_in == 0);
  return ret;
}

int zs_inflate(char* stream, void* out, int* out_bytes, int* consumed_input) {
  zng_stream* zs = (zng_stream*)stream;
  if (zs->avail_in == 0) {
    abort();
  }
  zs->next_out = out;
  zs->avail_out = *out_bytes;
  int ret = zng_inflate((zng_stream*)stream, Z_NO_FLUSH);
  if (ret == Z_OK || ret == Z_STREAM_END) {
    *out_bytes = zs->avail_out;
  }
  *consumed_input = (zs->avail_in == 0);
  return ret;
}

int zs_deflate_init(char* stream, int level) {
  zng_stream* zs = (zng_stream*)stream;
  memset(zs, 0, sizeof(*zs));
  return zng_deflateInit2(zs, level, Z_DEFLATED, 16 + 15, 8, Z_DEFAULT_STRATEGY);
}

int zs_deflate_with_input(char* stream, void* in, int in_bytes, void* out,
                          int* out_bytes) {
  zng_stream* zs = (zng_stream*)stream;
  if (zs->avail_in != 0 || in_bytes <= 0) {
    abort();
  }
  zs->avail_in = in_bytes;
  zs->next_in = in;
  zs->next_out = out;
  zs->avail_out = *out_bytes;
  int ret = zng_deflate(zs, Z_NO_FLUSH);
  *out_bytes = zs->avail_out;
  return ret;
}

int zs_deflate(char* stream, void* out, int* out_bytes) {
  zng_stream* zs = (zng_stream*)stream;
  if (zs->avail_in == 0) {
    abort();
  }
  zs->next_out = out;
  zs->avail_out = *out_bytes;
  int ret = zng_deflate(zs, Z_NO_FLUSH);
  *out_bytes = zs->avail_out;
  return ret;
}

int zs_deflate_end(char* stream, void* out, int* out_bytes) {
  zng_stream* zs = (zng_stream*)stream;
  if (zs->avail_in != 0) {
    abort();
  }
  zs->next_out = out;
  zs->avail_out = *out_bytes;
  int ret = zng_deflate(zs, Z_FINISH);
  *out_bytes = zs->avail_out;
  if (ret != Z_OK) {
    zng_deflateEnd(zs);
  }
  return ret;
}
