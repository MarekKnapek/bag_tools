#include "bag.h"

#include <cstdint>


static unsigned g_do_not_optimise;


extern "C" int LLVMFuzzerTestOneInput(std::uint8_t const* const data, std::size_t const size)
{
	mk::bag::records_t records;
	bool const parsed = mk::bag::parse(data, size, &records);
	g_do_not_optimise += parsed ? 1 : 0;
	g_do_not_optimise += static_cast<int>(records.size());
	return 0;
}


#include "bag.cpp"
#include "bag2.cpp"
#include "bag_to_pcap.cpp"
#include "bag_to_pcap_impl.cpp"
#include "bag_tool_info.cpp"
#include "bag_tool_info_impl.cpp"
#include "data_source_mem.cpp"
#include "data_source_rommf.cpp"
#include "read_only_memory_mapped_file.cpp"
#include "read_only_memory_mapped_file_linux.cpp"
#include "utils.cpp"


// clang++-10 -g -O1 -std=c++20 -DFUZZING -DNDEBUG -fsanitize=fuzzer,address bag_to_pcap/bag_to_pcap/src/bag_to_pcap_fuzz.cpp -o bag_to_pcap_fuzz.bin
// ./bag_to_pcap_fuzz.bin fuzz_corpus -jobs=7 -workers=7
