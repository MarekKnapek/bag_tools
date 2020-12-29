#pragma once


#ifdef _MSC_VER
	typedef wchar_t native_char_t;
	#define main_function wmain
#else
	typedef char native_char_t;
	#define main_function main
#endif
