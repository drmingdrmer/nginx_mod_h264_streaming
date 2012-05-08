/*******************************************************************************
 moov.c - A library for splitting Quicktime/MPEG4.

 Copyright (C) 2007-2009 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS // C++ should define this for PRIu64
#define __STDC_LIMIT_MACROS  // C++ should define this for UINT64_MAX
#endif

#include "moov.h"
#include "mp4_io.h"
#include "mp4_reader.h"

/* 
  The QuickTime File Format PDF from Apple:
    http://developer.apple.com/techpubs/quicktime/qtdevdocs/PDF/QTFileFormat.pdf
    http://developer.apple.com/documentation/QuickTime/QTFF/QTFFPreface/qtffPreface.html
*/

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef WIN32
#define stat _stat64
// #define strdup _strdup
#endif

char const* fragment_type_audio = "audio";
char const* fragment_type_video = "video";

char const* manifest_live = "live";
char const* manifest_vod = "vod";

/* Returns true when the test string is a prefix of the input */
int starts_with(const char* input, const char* test)
{
  while(*input && *test)
  {
    if(*input != *test)
      return 0;
    ++input;
    ++test;
  }

  return *test == '\0';
}

/* Returns true when the test string is a suffix of the input */
int ends_with(const char* input, const char* test)
{
  const char* it = input + strlen(input);
  const char* pit = test + strlen(test);
  while(it != input && pit != test)
  {
    if(*it != *pit)
      return 0;
    --it;
    --pit;
  }

  return pit == test;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int time_to_sample(trak_t const* trak, uint64_t pts)
{
  unsigned int i = 0;
  for(i = 0; i != trak->samples_size_; ++i)
  {
    if(trak->samples_[i].pts_ >= pts)
    {
      break;
    }
  }

  return i;
}

// reported by everwanna:
// av out of sync because: 
// audio track 0 without stss, seek to the exact time. 
// video track 1 with stss, seek to the nearest key frame time.
//
// fixed:
// first pass we get the new aligned times for traks with an stss present
// second pass is for traks without an stss
static int get_aligned_start_and_end(mp4_context_t const* mp4_context,
                                     int64_t start, int64_t end,
                                     unsigned int* trak_sample_start,
                                     unsigned int* trak_sample_end)
{
  unsigned int pass;
  moov_t const* moov = mp4_context->moov;
  long moov_time_scale = moov->mvhd_->timescale_;

  for(pass = 0; pass != 2; ++pass)
  {
    unsigned int i;
    for(i = 0; i != moov->tracks_; ++i)
    {
      trak_t const* trak = moov->traks_[i];
      long trak_time_scale = trak->mdia_->mdhd_->timescale_;

      if(trak->samples_size_ == 0)
      {
        trak_sample_start[i] = 0;
        trak_sample_end[i] = 0;
        continue;
      }

      // get start
      {
        unsigned int sample_index = time_to_sample(trak,
            moov_time_to_trak_time(start, moov_time_scale, trak_time_scale));

        // backtrack to nearest keyframe
        if(!trak->samples_[sample_index].is_ss_)
        {
          while(sample_index && !trak->samples_[sample_index].is_ss_)
          {
            --sample_index;
          }
          start = trak_time_to_moov_time(trak->samples_[sample_index].pts_,
                                         moov_time_scale, trak_time_scale);
        }
        trak_sample_start[i] = sample_index;
//        MP4_INFO("ts=%"PRId64" (moov time)\n", start);
        MP4_INFO("ts=%.2f (seconds)\n",
                 trak->samples_[sample_index].pts_ / (float)trak_time_scale);
      }

      // get end
      {
        unsigned int sample_index;
        if(end == 0)
        {
          // The default is till-the-end of the track
          sample_index = trak->samples_size_;
        }
        else
        {
          sample_index = time_to_sample(trak,
              moov_time_to_trak_time(end, moov_time_scale, trak_time_scale));
        }

        // backtrack to nearest keyframe
        if(!trak->samples_[sample_index].is_ss_)
        {
          while(sample_index && !trak->samples_[sample_index].is_ss_)
          {
            --sample_index;
          }
          end = trak_time_to_moov_time(trak->samples_[sample_index].pts_,
                                       moov_time_scale, trak_time_scale);
        }
        trak_sample_end[i] = sample_index;
//        MP4_INFO("te=%"PRId64" (moov time)\n", end);
        MP4_INFO("te=%.2f (seconds)\n",
                 trak->samples_[sample_index].pts_ / (float)trak_time_scale);
      }
    }
  }

  MP4_INFO("final start=%"PRId64"\n", start);
  MP4_INFO("final end=%"PRId64"\n", end);

  if(end && start >= end)
  {
    return 0;
  }

  return 1;
}

////////////////////////////////////////////////////////////////////////////////

mp4_split_options_t* mp4_split_options_init()
{
  mp4_split_options_t* options = (mp4_split_options_t*)
    malloc(sizeof(mp4_split_options_t));
  options->client_is_flash = 0;
  options->start = 0.0;
  options->start_integer = 0;
  options->end = 0.0;
  options->adaptive = 0;
  options->fragments = 0;
  options->manifest = 0;
  options->output_format = OUTPUT_FORMAT_MP4;
  options->input_format = INPUT_FORMAT_MP4;
//  options->fragment_type = NULL;
  options->audio_fragment_bitrate = 0;
  options->video_fragment_bitrate = 0;
  options->fragment_track_id = 0;
  options->audio_fragment_start = UINT64_MAX;
  options->video_fragment_start = UINT64_MAX;
  options->seconds = 0;
  options->byte_offsets = 0;
  options->flv_header = 0;

  //
  options->stream_id[0] = '\0';

  return options;
}

int mp4_split_options_set(struct mp4_split_options_t* options,
                          const char* args_data,
                          unsigned int args_size)
{
  int result = 1;

  {
    const char* first = args_data;
    const char* last = first + args_size + 1;

    if(*first == '?')
    {
      ++first;
    }

    {
      char const* key = first;
      char const* val = NULL;
      int is_key = 1;
      size_t key_len = 0;

      float vbegin = 0.0f;
      float vend = 0.0f;

      unsigned int bitrate = 0;
      char const* fragment_type = NULL;

      while(first != last)
      {
        // the args_data is not necessarily 0 terminated, so fake it
        int ch = (first == last - 1) ? '\0' : *first;
        switch(ch)
        {
        case '=':
          val = first + 1;
          key_len = first - key;
          is_key = 0;
          break;
        case '&':
        case '\0':
          if(!is_key)
          {
            // make sure the value is zero-terminated (for strtod,atoi64)
            int val_len = first - val;
            char* valz = (char*)malloc(val_len + 1);
            memcpy(valz, val, val_len);
            valz[val_len] = '\0';

            if(!strncmp("client", key, key_len))
            {
              options->client_is_flash = starts_with(valz, "FLASH");
            } else
            if(!strncmp("ts", key, key_len))
            {
              options->start = (float)(strtod(valz, NULL));
              options->start_integer = atoi64(valz);
            } else
            if(!strncmp("te", key, key_len))
            {
              options->end = (float)(strtod(valz, NULL));
            } else
            if(!strncmp("vbegin", key, key_len))
            {
              vbegin = (float)(strtod(valz, NULL));
            } else
            if(!strncmp("vend", key, key_len))
            {
              vend = (float)(strtod(valz, NULL));
            } else
            if(!strncmp("adaptive", key, key_len))
            {
              options->adaptive = 1;
            } else
            if(!strncmp("bitrate", key, key_len))
            {
              bitrate = (unsigned int)(atoi64(valz));
              if(fragment_type)
              {
                if(!strcmp(fragment_type, fragment_type_video))
                {
                  options->video_fragment_bitrate = bitrate;
                } else
                if(!strcmp(fragment_type, fragment_type_audio))
                {
                  options->audio_fragment_bitrate = bitrate;
                }
              }
            } else
            if(!strncmp("video", key, key_len))
            {
              options->fragments = 1;
              fragment_type = fragment_type_video;
              options->video_fragment_start = atoi64(valz);
              if(bitrate)
              {
                options->video_fragment_bitrate = bitrate;
              }
            } else
            if(!strncmp("audio", key, key_len))
            {
              options->fragments = 1;
              fragment_type = fragment_type_audio;
              options->audio_fragment_start = atoi64(valz);
              if(bitrate)
              {
                options->audio_fragment_bitrate = bitrate;
              }
            } else
            if(!strncmp("format", key, key_len))
            {
              if(!strncmp("flv", val, val_len))
              {
                options->output_format = OUTPUT_FORMAT_FLV;
              } else
              if(!strncmp("mov", val, val_len))
              {
                options->output_format = OUTPUT_FORMAT_MOV;
              } else
              if(!strncmp("ts", val, val_len))
              {
                options->output_format = OUTPUT_FORMAT_TS;
              }
            } else
            if(!strncmp("input", key, key_len))
            {
              if(!strncmp("flv", val, val_len))
              {
                options->input_format = INPUT_FORMAT_FLV;
              }
            } else
            if(!strncmp("manifest", key, key_len))
            {
              if(!strncmp("live", val, val_len))
              {
                options->manifest = manifest_live;
              } else
              if(!strncmp("vod", val, val_len))
              {
                options->manifest = manifest_vod;
              } else
              {
                return 0;
              }
            } else
            if(!strncmp("flv_header", key, key_len))
            {
              options->flv_header = atoi(valz);
            } else
            if(!strncmp("stream_id", key, key_len))
            {
              strncpy(options->stream_id, valz, sizeof(options->stream_id) - 1);
              options->stream_id[sizeof(options->stream_id) - 1] = '\0';
            }
            free(valz);
          }
          key = first + 1;
          val = NULL;
          is_key = 1;
          break;
        }
        ++first;
      }

      // If we have specified a begin point of the virtual video clip,
      // then adjust the start offset
      options->start += vbegin;

      // If we have specified an end, adjust it in case of a virtual video clip.
      if(options->end)
      {
        options->end += vbegin;
      }
      else
      {
        options->end = vend;
      }

      // Validate the start/end for the virtual video clip (begin).
      if(vbegin)
      {
        if(options->start < vbegin)
          result = 0;
        if(options->end && options->end < vbegin)
          result = 0;
      }
      // Validate the start/end for the virtual video clip (end).
      if(vend)
      {
        if(options->start > vend)
          result =  0;
        if(options->end && options->end > vend)
          result = 0;
      }
    }
  }

  return result;
}

void mp4_split_options_exit(struct mp4_split_options_t* options)
{
  if(options->byte_offsets)
  {
    free(options->byte_offsets);
  }

  free(options);
}

extern int mp4_split(struct mp4_context_t* mp4_context,
                     unsigned int* trak_sample_start,
                     unsigned int* trak_sample_end,
                     mp4_split_options_t const* options)
{
  int result;

  float start_time = options->start;
  float end_time = options->end;

//  moov_build_index(mp4_context, mp4_context->moov);

  {
    struct moov_t const* moov = mp4_context->moov;
    long moov_time_scale = moov->mvhd_->timescale_;
    int64_t start = (int64_t)(start_time * moov_time_scale + 0.5f);
    int64_t end = (int64_t)(end_time * moov_time_scale + 0.5f);

    // for every trak, convert seconds to sample (time-to-sample).
    // adjust sample to keyframe
    result = get_aligned_start_and_end(mp4_context, start, end,
                                       trak_sample_start, trak_sample_end);
  }

  return result;
}

uint64_t get_filesize(const char *path)
{
  struct stat status;
  if(stat(path, &status))
  {
    printf("get_filesize(%s): %s\n", path, strerror(errno));
    return 0;
  }
  return status.st_size;
}

// End Of File

