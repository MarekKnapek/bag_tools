#include "bag_to_pcap.h"

#include "bag_to_pcap_impl.h"


bool mk::bag_tool::bag_to_pcap(int const argc, native_char_t const* const* const argv)
{
	return mk::bag_tool::detail::bag_to_pcap(argc, argv);
}
