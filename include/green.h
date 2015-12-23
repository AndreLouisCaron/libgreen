////////////////////////////////////////////////////////////////////////
// Copyright(c) libgreen contributors.  See LICENSE file for details. //
////////////////////////////////////////////////////////////////////////

#ifndef _GREEN_H__
#define _GREEN_H__

// Library version.
#define GREEN_MAJOR 0
#define GREEN_MINOR 1
#define GREEN_PATCH 0

// Compile-time version check helper.
#define GREEN_MAKE_VERSION(major, minor, patch) \
    ((major * 10000) + (minor * 100) + patch)

// Helper macro.
#define _GREEN_STRING(x) #x
#define GREEN_STRING(x) _GREEN_STRING(x)

// API version.
#define GREEN_VERSION \
    GREEN_MAKE_VERSION(GREEN_MAJOR, GREEN_MINOR, GREEN_PATCH)
#define GREEN_VERSION_STRING \
    GREEN_STRING(GREEN_MAJOR) "." \
    GREEN_STRING(GREEN_MINOR) "." \
    GREEN_STRING(GREEN_PATCH)

// Lib version.
int green_version();
const char * green_version_string();

// Library setup and teardown.
int _green_init(int major, int minor);
#define green_init() \
    _green_init(GREEN_MAJOR, GREEN_MINOR)
int green_term();

#endif // _GREEN_H__
