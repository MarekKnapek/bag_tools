#include "bag_tool_info.h"

#include "bag_tool_info_impl.h"


bool mk::bag_tool::bag_info(int const argc, native_char_t const* const* const argv)
{
	return mk::bag_tool::detail::bag_info(argc, argv);
}
