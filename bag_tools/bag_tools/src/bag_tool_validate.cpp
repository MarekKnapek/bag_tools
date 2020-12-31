#include "bag_tool_validate.h"

#include "bag_tool_validate_impl.h"


bool mk::bag_tool::bag_validate(int const argc, native_char_t const* const* const argv)
{
	return mk::bag_tool::detail::bag_validate(argc, argv);
}
