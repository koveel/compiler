#pragma once

#include "Logging.h"

#define LOG(x, ...) Log::print(##x, __VA_ARGS__)

#ifndef COMPILER_RELEASE
	#define ASSERT(x) { if (!(x)) { LOG(#x); __debugbreak(); } }
#else
	#define ASSERT(x)
#endif