/*******************************************************************************
 mp4_process.c - .

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined(BUILDING_NGINX)
#include "mp4_process.h"
#endif
#include "moov.h"

#include "mp4_io.h"
#include "mp4_reader.h"
#include "output_bucket.h"

#if defined(BUILDING_H264_STREAMING) || defined(BUILDING_MP4SPLIT)
#include "output_mp4.h"
#endif
#if defined(BUILDING_MP4SPLIT)
#include "output_mov.h"
#endif
#if defined(BUILDING_SMOOTH_STREAMING) || defined(BUILDING_MP4SPLIT)
#include "output_ismv.h"
#include "output_ts.h"
#include "output_fflv.h"
#endif
#if defined(BUILDING_LIVE_SMOOTH_STREAMING)
#include "mp4_pubpoint.h"
#endif
#if defined(BUILDING_FLV_STREAMING) || defined(BUILDING_MP4SPLIT)
#include "output_flv.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#if defined(BUILDING_NGINX)
static
#else
extern
#endif
int mp4_process(char const* filename, uint64_t filesize, int verbose,
                buckets_t* buckets,
                mp4_split_options_t* options)
{
  int result = 1;

  // implement mod_flv_streaming for convenience
  if(ends_with(filename, ".flv") || options->input_format == INPUT_FORMAT_FLV)
  {
    static const unsigned char flv_header[13] = {
			'F', 'L', 'V', 0x01, 0x01, 0x00, 0x00, 0x00, 0x09,
      0x00, 0x00, 0x00, 0x09
    };
    uint64_t start = options->start_integer;

    buckets->content_type_ = mime_type_flv;

    if(start != 0)
    {
      bucket_insert_tail(buckets, bucket_init_memory(buckets, flv_header, 13));
    }
    bucket_insert_tail(buckets,
      bucket_init_file(buckets, filename, start, filesize - start));

    return 200; // HTTP_OK;
  }

#if defined(BUILDING_LIVE_SMOOTH_STREAMING)
  if(options->manifest == manifest_live)
  {
    int is_live = 1;
    buckets->content_type_ = mime_type_ismc;
    return pubpoint_output_ismc(filename, buckets, is_live);
  }
#endif

  if(options->fragments)
  {
    if(0) { }
#if defined(HAVE_OUTPUT_TS)
    else if(options->output_format == OUTPUT_FORMAT_TS)
    {
      buckets->content_type_ = mime_type_ts;
      result = output_ts(filename, buckets, options);
    }
#endif
#if defined(HAVE_OUTPUT_ISMV)
    else if(options->output_format == OUTPUT_FORMAT_MP4 ||
            options->output_format == OUTPUT_FORMAT_MOV)
    {
      result = output_ismv(filename, buckets, options);
    }
#endif
#if defined(HAVE_OUTPUT_FFLV)
    else if(options->output_format == OUTPUT_FORMAT_FLV)
    {
      buckets->content_type_ = mime_type_flv;
      result = output_fflv(filename, buckets, options);
    }
#endif
    else
    {
      result = 0;
    }
  }
  else
  {
    // Open the file
    mp4_open_flags flags = options->fragments ? MP4_OPEN_MFRA : MP4_OPEN_ALL;
    mp4_context_t* mp4_context = mp4_open(filename, filesize, flags, verbose);

    if(mp4_context == NULL)
    {
      result = 0;
    }

    if(result)
    {
      // split the movie
      unsigned int trak_sample_start[MAX_TRACKS];
      unsigned int trak_sample_end[MAX_TRACKS];
      result = mp4_split(mp4_context, trak_sample_start, trak_sample_end,
                         options);
      if(result)
      {
        if(0)
        {
        }
#ifdef HAVE_OUTPUT_FLV
        else if(options->output_format == OUTPUT_FORMAT_FLV)
        {
          result = output_flv(mp4_context,
                              trak_sample_start,
                              trak_sample_end,
                              buckets, options);
        }
#endif
#ifdef HAVE_OUTPUT_MP4
        else if(options->output_format == OUTPUT_FORMAT_MP4)
        {
          result = output_mp4(mp4_context,
                              trak_sample_start,
                              trak_sample_end,
                              buckets, options);
        }
#endif
#ifdef HAVE_OUTPUT_MOV
        else if(options->output_format == OUTPUT_FORMAT_MOV)
        {
          result = output_mov(mp4_context,
                              trak_sample_start,
                              trak_sample_end,
                              buckets, options);
        }
#endif
      }
    }

    // close the file
    if(mp4_context)
    {
      mp4_close(mp4_context);
    }
  }

  if(!result)
  {
    return 415; // HTTP_UNSUPPORTED_MEDIA_TYPE;
  }

  return 200; // HTTP_OK;
}

// End Of File

