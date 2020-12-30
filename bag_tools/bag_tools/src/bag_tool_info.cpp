#include "bag_tool_info.h"

#include "bag_tool_info_impl.h"


bool mk::bag_tools::bag_info(native_char_t const* const input_bag)
{
	return mk::bag_tools::detail::bag_info(input_bag);
}
