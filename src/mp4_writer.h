/*******************************************************************************
 mp4_writer.h - A library for writing MPEG4.

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifndef MP4_WRITER_H_AKW
#define MP4_WRITER_H_AKW

#include "mod_streaming_export.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

struct unknown_atom_t;
struct moov_t;
struct trak_t;
struct mfra_t;
struct moof_t;

struct atom_write_list_t
{
  uint32_t type_;
  void const* source_;
  unsigned char* (*writer_)(void const* atom, unsigned char* buffer);
};
typedef struct atom_write_list_t atom_write_list_t;
MOD_STREAMING_DLL_LOCAL extern
unsigned char* atom_writer(struct unknown_atom_t* unknown_atoms,
                           atom_write_list_t* atom_write_list,
                           unsigned int atom_write_list_size,
                           unsigned char* buffer);

MOD_STREAMING_DLL_LOCAL extern
void trak_shift_offsets_inplace(struct trak_t* trak, int64_t offset);

MOD_STREAMING_DLL_LOCAL extern
uint32_t moov_write(struct moov_t* atom, unsigned char* buffer);

MOD_STREAMING_DLL_LOCAL extern
uint32_t mfra_write(struct mfra_t const* mfra, unsigned char* buffer);

MOD_STREAMING_DLL_LOCAL extern
uint32_t moof_write(struct moof_t* atom, unsigned char* buffer);

MOD_STREAMING_DLL_LOCAL extern
unsigned char* uuid0_write(void const* atom, unsigned char* buffer);

MOD_STREAMING_DLL_LOCAL extern
unsigned char* uuid1_write(void const* atom, unsigned char* buffer);

#ifdef __cplusplus
} /* extern C definitions */
#endif

#endif // MP4_WRITER_H_AKW

// End Of File

