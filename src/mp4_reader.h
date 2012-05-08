/*******************************************************************************
 mp4_reader.h - A library for reading MPEG4.

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifndef MP4_READER_H_AKW
#define MP4_READER_H_AKW

#include "mod_streaming_export.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mp4_context_t;
struct moov_t;

struct atom_read_list_t
{
  uint32_t type_;
  int (*destination_)(struct mp4_context_t const* mp4_context,
                      void* parent, void* child);
  void* (*reader_)(struct mp4_context_t const* mp4_context,
                   void* parent, unsigned char* buffer, uint64_t size);
};
typedef struct atom_read_list_t atom_read_list_t;
MOD_STREAMING_DLL_LOCAL extern
int atom_reader(struct mp4_context_t const* mp4_context,
                struct atom_read_list_t* atom_read_list,
                unsigned int atom_read_list_size,
                void* parent,
                unsigned char* buffer, uint64_t size);

MOD_STREAMING_DLL_LOCAL extern
void* moov_read(struct mp4_context_t const* mp4_context,
                void* parent,
                unsigned char* buffer, uint64_t size);

MOD_STREAMING_DLL_LOCAL extern
void* moof_read(struct mp4_context_t const* mp4_context,
                void* parent,
                unsigned char* buffer, uint64_t size);

// MOD_STREAMING_DLL_LOCAL extern
// int moov_build_index(struct mp4_context_t const* mp4_context,
//                      struct moov_t* moov);

MOD_STREAMING_DLL_LOCAL extern
void* mfra_read(struct mp4_context_t const* mp4_context,
                void* parent,
                unsigned char* buffer, uint64_t size);

enum mp4_open_flags
{
  MP4_OPEN_MOOV = 0x00000001,
  MP4_OPEN_MOOF = 0x00000002,
  MP4_OPEN_MDAT = 0x00000004,
  MP4_OPEN_MFRA = 0x00000008,
  MP4_OPEN_ALL  = 0x0000000f
};
typedef enum mp4_open_flags mp4_open_flags;

MOD_STREAMING_DLL_LOCAL extern
struct mp4_context_t* mp4_open(const char* filename, int64_t filesize,
                               mp4_open_flags flags, int verbose);

MOD_STREAMING_DLL_LOCAL extern void mp4_close(struct mp4_context_t* mp4_context);

#ifdef __cplusplus
} /* extern C definitions */
#endif

#endif // MP4_READER_H_AKW

// End Of File

