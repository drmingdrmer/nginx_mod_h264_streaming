/*******************************************************************************
 output_bucket.h - A library for writing memory / file buckets.

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifndef OUTPUT_BUCKET_H_AKW
#define OUTPUT_BUCKET_H_AKW

#include "mod_streaming_export.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

enum bucket_type_t
{
  BUCKET_TYPE_MEMORY,
  BUCKET_TYPE_FILE
};
typedef enum bucket_type_t bucket_type_t;

struct bucket_t
{
  int type_;
//  union {
    void* buf_;
    char const* filename_;
    uint64_t offset_;
//  };
  uint64_t size_;
  struct bucket_t* prev_;
  struct bucket_t* next_;
};
typedef struct bucket_t bucket_t;

struct pool_entry_t;

struct buckets_t
{
  const char* content_type_;
  bucket_t* bucket_;
  struct pool_entry_t* pool_;
};
typedef struct buckets_t buckets_t;

MOD_STREAMING_DLL_LOCAL extern bucket_t* bucket_init(bucket_type_t bucket_type);
MOD_STREAMING_DLL_LOCAL extern void bucket_exit(bucket_t* bucket);
MOD_STREAMING_DLL_LOCAL extern
bucket_t* bucket_init_memory(buckets_t* buckets, void const* buf, uint64_t size);
MOD_STREAMING_DLL_LOCAL extern
bucket_t* bucket_init_file(buckets_t* buckets, char const* filename, uint64_t offset, uint64_t size);
MOD_STREAMING_DLL_LOCAL extern
buckets_t* buckets_init();
MOD_STREAMING_DLL_LOCAL extern
void buckets_exit(buckets_t* buckets);
MOD_STREAMING_DLL_LOCAL extern
void bucket_insert_tail(buckets_t* buckets, bucket_t* bucket);
MOD_STREAMING_DLL_LOCAL extern
void bucket_insert_head(buckets_t* buckets, bucket_t* bucket);
MOD_STREAMING_DLL_LOCAL extern
void bucket_remove(bucket_t* bucket);

// utility functions

MOD_STREAMING_DLL_LOCAL extern
uint64_t buckets_size(buckets_t* buckets, bucket_t* end_bucket);

MOD_STREAMING_DLL_LOCAL extern
void* buckets_flatten(buckets_t* buckets, uint64_t* size, bucket_t* end_bucket);

#ifdef __cplusplus
} /* extern C definitions */
#endif

#endif // OUTPUT_BUCKET_H_AKW

// End Of File

