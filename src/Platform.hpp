#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#define CENTAURUS_CALLBACK __cdecl
#else
#define CENTAURUS_CALLBACK 
#endif

#if defined(CENTAURUS_BUILD_LINUX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#endif

namespace Centaurus
{

  inline int get_current_pid() {
#if defined(CENTAURUS_BUILD_WINDOWS)
    return GetCurrentProcessId();
#elif defined(CENTAURUS_BUILD_LINUX)
    return getpid();
#endif
  }

}
