#ifndef _DEFINES_H
#define _DEFINES_H

#ifdef PLATFORM_UNIX
#include <unistd.h>
#endif

#ifdef PLATFORM_WIN32
#define usleep(value) sleep(value)
#endif

#endif
