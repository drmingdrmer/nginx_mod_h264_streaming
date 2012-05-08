/*******************************************************************************
 output_bucket.c - A library for writing memory / file buckets.

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "output_bucket.h"
#include "mp4_io.h"
#include <stdlib.h>
#include <string.h>

struct pool_entry_t
{
  char* key_;
  struct pool_entry_t* next_;
};
typedef struct pool_entry_t pool_entry_t;

static char const* pool_insert(buckets_t* buckets, char const* str)
{
  pool_entry_t* ptr = buckets->pool_;
  while(ptr)
  {
    if(!strcmp(ptr->key_, str))
    {
      return ptr->key_;
    }
    ptr = ptr->next_;
  }

  ptr = (pool_entry_t*)malloc(sizeof(pool_entry_t));
  ptr->key_ = strdup(str);
  ptr->next_ = buckets->pool_;

  buckets->pool_ = ptr;

  return ptr->key_;
}

static void pool_exit(pool_entry_t* pool)
{
  while(pool)
  {
    pool_entry_t* next = pool->next_;
    free(pool->key_);
    free(pool);
    pool = next;
  }
}

extern bucket_t* bucket_init(enum bucket_type_t bucket_type)
{
  bucket_t* bucket = (bucket_t*)malloc(sizeof(bucket_t));
  bucket->type_ = bucket_type;
  bucket->prev_ = bucket;
  bucket->next_ = bucket;

  return bucket;
}

extern void bucket_exit(bucket_t* bucket)
{
  switch(bucket->type_)
  {
  case BUCKET_TYPE_MEMORY:
    free(bucket->buf_);
    break;
  case BUCKET_TYPE_FILE:
    break;
  }
  free(bucket);
}

extern bucket_t* bucket_init_memory(buckets_t* UNUSED(buckets), void const* buf,
                                    uint64_t size)
{
  bucket_t* bucket = bucket_init(BUCKET_TYPE_MEMORY);
  bucket->buf_ = malloc((size_t)size);
  memcpy(bucket->buf_, buf, (size_t)size);
  bucket->size_ = size;
  return bucket;
}

extern bucket_t* bucket_init_file(buckets_t* buckets, char const* filename,
                                  uint64_t offset, uint64_t size)
{
  bucket_t* bucket = bucket_init(BUCKET_TYPE_FILE);
  bucket->filename_ = pool_insert(buckets, filename);
  bucket->offset_ = offset;
  bucket->size_ = size;
  return bucket;
}

static void bucket_insert_after(bucket_t* after, bucket_t* bucket)
{
  bucket->prev_ = after;
  bucket->next_ = after->next_;
  after->next_->prev_ = bucket;
  after->next_ = bucket;
}

extern void bucket_insert_tail(buckets_t* buckets, bucket_t* bucket)
{
  bucket_t** head = &buckets->bucket_;

  if(*head == NULL)
  {
    *head = bucket;
  }

  bucket_insert_after((*head)->prev_, bucket);
}

extern void bucket_insert_head(buckets_t* buckets, bucket_t* bucket)
{
  bucket_insert_tail(buckets, bucket);
  buckets->bucket_ = bucket;
}

extern void bucket_remove(bucket_t* bucket)
{
  bucket_t* prev = bucket->prev_;
  bucket_t* next = bucket->next_;
  bucket->prev_->next_ = next;
  bucket->next_->prev_ = prev;
}

extern buckets_t* buckets_init()
{
  buckets_t* buckets = (buckets_t*)malloc(sizeof(buckets_t));

  buckets->content_type_ = mime_type_mp4;
  buckets->bucket_ = NULL;
  buckets->pool_ = NULL;

  return buckets;
}

extern void buckets_exit(buckets_t* buckets)
{
  bucket_t* bucket = buckets->bucket_;
  if(bucket)
  {
    do
    {
      bucket_t* next = bucket->next_;
      bucket_exit(bucket);
      bucket = next;
    } while(bucket != buckets->bucket_);
  }

  pool_exit(buckets->pool_);

  free(buckets);
}

extern uint64_t buckets_size(buckets_t* buckets, bucket_t* end_bucket)
{
  bucket_t* bucket = buckets->bucket_;
  uint64_t size = 0;

  if(bucket == NULL)
  {
    return 0;
  }

  if(end_bucket == NULL)
  {
    end_bucket = bucket;
  }

  do
  {
    size += bucket->size_;
    bucket = bucket->next_;
  } while(bucket != end_bucket);

  return size;
}

extern void* buckets_flatten(buckets_t* buckets, uint64_t* size,
                             bucket_t* end_bucket)
{
  bucket_t* bucket = buckets->bucket_;

  if(bucket == NULL)
  {
    return 0;
  }

  if(end_bucket == NULL)
  {
    end_bucket = bucket;
  }

  {
    uint64_t filesize = buckets_size(buckets, end_bucket);

    *size = filesize;

    {
      unsigned char* dst = (unsigned char*)malloc((size_t)filesize);
      uint64_t dst_offset = 0;
      int result = 1;
//      bucket_t* bucket = buckets->bucket_;

      do
      {
        unsigned char const* src;
        uint64_t offset;
        for(offset = 0; offset != bucket->size_;)
        {
          uint32_t chunk_size = 16 * 1024 * 1024;
          if(bucket->size_ - offset < chunk_size)
          {
            chunk_size = bucket->size_ - offset < chunk_size;
          }

          if(bucket->type_ == BUCKET_TYPE_MEMORY)
          {
            src = (unsigned char const*)(bucket->buf_) + offset;
          } else // if(bucket->type_ == BUCKET_TYPE_FILE)
          {
            mem_range_t* src_mem_range = mem_range_init_read(bucket->filename_);

            if(!src_mem_range)
            {
              return 0;
            }

            src = (unsigned char const*)
              (mem_range_map(src_mem_range, bucket->offset_ + offset, chunk_size));

            mem_range_exit(src_mem_range);
          }

          memcpy(dst + dst_offset, src, chunk_size);
          offset += chunk_size;
          dst_offset += chunk_size;
        }
        bucket = bucket->next_;
      } while(bucket != end_bucket && result);

//      buckets_exit(buckets);

      return dst;
    }
  }
}

// End Of File

