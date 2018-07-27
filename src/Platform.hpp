#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#define CENTAURUS_CALLBACK __cdecl
#else
#define CENTAURUS_CALLBACK 
#endif
