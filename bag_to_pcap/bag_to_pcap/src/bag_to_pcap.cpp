#include "bag_to_pcap.h"

#include "bag_to_pcap_impl.h"


bool mk::bag_tools::bag_to_pcap(native_char_t const* const input_bag, native_char_t const* const output_pcap)
{
	return mk::bag_tools::detail::bag_to_pcap(input_bag, output_pcap);
}
