#include "bag_to_pcap.h"
#include "cross_platform.h"
#include "scope_exit.h"
#include "utils.h"

#include <cstdio> // std::puts
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS


bool bag_to_pcap(int const argc, native_char_t const* const* const argv);
bool bag_to_pcap(native_char_t const* const input_file_name, native_char_t const* const output_file_name);


int main_function(int const argc, native_char_t const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([](){ std::puts("Oh no! Something went wrong!"); });

	bool const bussiness = bag_to_pcap(argc, argv);
	CHECK_RET(bussiness, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash. Great success!");
	return EXIT_SUCCESS;
}


bool bag_to_pcap(int const argc, native_char_t const* const* const argv)
{
	CHECK_RET_F(argc == 3);

	bool const bussiness = bag_to_pcap(argv[1], argv[2]);
	CHECK_RET_F(bussiness);

	return true;
}

bool bag_to_pcap(native_char_t const* const input_file_name, native_char_t const* const output_file_name)
{
	bool const bussiness = mk::bag_tools::bag_to_pcap(input_file_name, output_file_name);
	CHECK_RET_F(bussiness);

	return true;
}
