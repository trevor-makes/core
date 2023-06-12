// Copyright (c) 2022 Trevor Makes

#pragma once

// On Arduino, string tables are stored in Flash memory to save RAM, but as a
// consequence special macros must to be used to differentiate Flash pointers
// from RAM pointers.

// On other platforms (e.g. unit testing on desktop) we just store the string
// tables in regular RAM and fake the Flash memory macros.

// strings.h is POSIX; may need to use _stricmp instead of strcasecmp on Windows
#include <strings.h>

#define PROGMEM
#define __FlashStringHelper char
#define F(x) (x)

#define pgm_read_byte(ptr) (*(ptr))
#define pgm_read_ptr(ptr) (*(ptr))

#define strcmp_P strcmp
#define strcasecmp_P strcasecmp
