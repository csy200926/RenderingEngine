#pragma once
#include <cstdio>
#include <cstdarg>
#define DEBUG_PRINT(...) MyEngine::consolePrint(__VA_ARGS__)
namespace MyEngine
{
	inline void consolePrint(const char *i_format, ...)
	{
		va_list args;
		va_start(args, i_format);
		vprintf(i_format, args);
		va_end(args);
	}
} // namespace MyEngine
