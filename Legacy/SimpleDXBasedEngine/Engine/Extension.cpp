#include "Extension.h"
#include "Windows.h"
#include "stdio.h"

namespace MyEngine{

	void consolePrint(const char * i_fmt,...)
	{

		// Space
		const unsigned int str_length = 512;
		char str[str_length];

		// Pointer start
		va_list arg;
		va_start(arg,i_fmt);

		// build the string by using vsprintf_s
		vsprintf_s(str,str_length,i_fmt,arg);

		va_end(arg);

		// print out!
		OutputDebugStringA(str);

	}








}