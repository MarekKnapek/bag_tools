#pragma once


#ifdef _MSC_VER
	#include <cwchar> // std::wcslen
	typedef wchar_t native_char_t;
	#define main_function wmain
	#define native_strlen std::wcslen
	#define MK_TEXT(X) L ## X
#else
	#include <cstring> // std::strlen
	typedef char native_char_t;
	#define main_function main
	#define native_strlen std::strlen
	#define MK_TEXT(X) X
#endif
