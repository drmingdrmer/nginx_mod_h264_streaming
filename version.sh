#! /bin/bash
#
#

grep 'X_MOD_H264_STREAMING_VERSION' ./src/mod_streaming_export.h | sed 's/.*"version=\(.*\)".*/\1/'

