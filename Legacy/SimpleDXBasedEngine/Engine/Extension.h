#pragma once

#ifdef _DEBUG
#define DEBUG_PRINT(fmt,...) MyEngine::consolePrint((fmt),__VA_ARGS__);
#else
#define DEBUG_PRINT(fmt,...) void(0);
#endif



namespace MyEngine
{
	void consolePrint(const char * i_fmt,...);
}