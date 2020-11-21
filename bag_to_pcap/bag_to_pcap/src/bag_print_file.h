#pragma once


#include "cross_platform.h"

#include <cstdint> // std::uint64_t


namespace mk
{
	namespace bag
	{


		bool print_file(native_char_t const* const& file_path);
		bool print_records(void const* const& data, std::uint64_t const& len);


	}
}
