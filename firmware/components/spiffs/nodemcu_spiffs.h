#ifndef _NODEMCU_SPIFFS_H
#define _NODEMCU_SPIFFS_H

#ifndef NODEMCU_SPIFFS_NO_INCLUDE
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#endif

// Turn off stats
#define SPIFFS_CACHE_STATS 	    0
#define SPIFFS_GC_STATS             0

// Needs to align stuff
#define SPIFFS_ALIGNED_OBJECT_INDEX_TABLES	1

// Enable magic so we can find the file system (but not yet)
#define SPIFFS_USE_MAGIC            1
#define SPIFFS_USE_MAGIC_LENGTH     1

// Reduce the chance of returning disk full
#define SPIFFS_GC_MAX_RUNS          256

#endif
