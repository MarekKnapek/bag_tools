#include "bag_to_pcap.h"
#include "bag_tool_info.h"
#include "cross_platform.h"
#include "scope_exit.h"
#include "utils.h"

#include <cstdio> // std::puts
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memcmp
#include <iterator> // std::size


static constexpr native_char_t const s_tool_info_name[] = MK_TEXT("/info");
static constexpr int const s_tool_info_name_len = static_cast<int>(std::size(s_tool_info_name)) - 1;
static constexpr native_char_t const s_tool_pcap_name[] = MK_TEXT("/pcap");
static constexpr int const s_tool_pcap_name_len = static_cast<int>(std::size(s_tool_pcap_name)) - 1;


bool do_bussiness(int const argc, native_char_t const* const* const argv);


int main_function(int const argc, native_char_t const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([](){ std::puts("Oh no! Something went wrong!"); });

	bool const bussiness = do_bussiness(argc, argv);
	CHECK_RET(bussiness, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash. Great success!");
	return EXIT_SUCCESS;
}


bool do_bussiness(int const argc, native_char_t const* const* const argv)
{
	if(argc == 1)
	{
		std::puts
		(
			"Welcome to bag tools by Marek Knapek.\n"
			"https://github.com/MarekKnapek\n"
			"\n"
			"Usage: <command> <command param 1> <command param 2> ...\n"
			"\n"
			"Commands:\n"
			"\t/info\t Prints info about bag file.\n"
			"\t/pcap\t Converts Ouster LiDAR capture file from bag to pcap format.\n"
			"\n"
			"Example usage:\n"
			"\tbag_tools.exe /info input.bag\n"
			"\tbag_tools.exe /pcap input.bag output.pcap\n"
		);
		return true;
	}

	native_char_t const* const command = argv[1];
	int const command_len = static_cast<int>(native_strlen(command));
	if(command_len == s_tool_info_name_len && std::memcmp(command, s_tool_info_name, s_tool_info_name_len * sizeof(native_char_t)) == 0)
	{
		bool const command_ret = mk::bag_tool::bag_info(argc, argv);
		CHECK_RET_F(command_ret);
	}
	else if(command_len == s_tool_pcap_name_len && std::memcmp(command, s_tool_pcap_name, s_tool_pcap_name_len * sizeof(native_char_t)) == 0)
	{
		bool const command_ret = mk::bag_tool::bag_to_pcap(argc, argv);
		CHECK_RET_F(command_ret);
	}
	else
	{
		return false;
	}

	return true;
}
