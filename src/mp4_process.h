/*******************************************************************************
 mp4_process.h -

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifndef MP4_PROCESS_H_AKW
#define MP4_PROCESS_H_AKW

#include "mod_streaming_export.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct buckets_t;
struct mp4_split_options_t;

MOD_STREAMING_DLL_LOCAL extern
int mp4_process(char const* filename, uint64_t filesize, int verbose,
                struct buckets_t* buckets,
                struct mp4_split_options_t* options);

#ifdef __cplusplus
} /* extern C definitions */
#endif

#endif // MP4_PROCESS_H_AKW

// End Of File

