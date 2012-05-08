/*******************************************************************************
 mod_streaming_export.h

 Copyright (C) 2009-2010 CodeShop B.V.
 http://www.code-shop.com

 For licensing see the LICENSE file
******************************************************************************/ 

#ifndef MOD_STREAMING_EXPORT_H_AKW
#define MOD_STREAMING_EXPORT_H_AKW

#if defined _WIN32 || defined __CYGWIN__
  #define MOD_STREAMING_DLL_IMPORT __declspec(dllimport)
  #define MOD_STREAMING_DLL_EXPORT __declspec(dllexport)
  #define MOD_STREAMING_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define MOD_STREAMING_DLL_IMPORT __attribute__ ((visibility("default")))
    #define MOD_STREAMING_DLL_EXPORT __attribute__ ((visibility("default")))
    #define MOD_STREAMING_DLL_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define MOD_STREAMING_DLL_IMPORT
    #define MOD_STREAMING_DLL_EXPORT
    #define MOD_STREAMING_DLL_LOCAL
  #endif
#endif

#define X_MOD_H264_STREAMING_KEY       "X-Mod-H264-Streaming"
#define X_MOD_H264_STREAMING_VERSION   "version=2.3.2"

#define X_MOD_SMOOTH_STREAMING_KEY     "X-Mod-Smooth-Streaming"
#define X_MOD_SMOOTH_STREAMING_VERSION "version=1.3.2"

#ifdef __cplusplus
#define __STDC_FORMAT_MACROS // C++ should define this for PRIu64
#define __STDC_LIMIT_MACROS  // C++ should define this for UINT64_MAX
#define __STDC_CONSTANT_MACROS // C++ should define this for INT64_C
#endif

#if defined(_MSC_VER)
# if !defined(NDEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <crtdbg.h>
# endif
# define snprintf _snprintf
# if !defined(strdup)
# define strdup _strdup
#endif
#endif

// #if defined(HAVE_DB_H)
// #define BUILDING_LIVE_SMOOTH_STREAMING
// #endif

#endif // MOD_STREAMING_EXPORT_H_AKW

// End Of File

