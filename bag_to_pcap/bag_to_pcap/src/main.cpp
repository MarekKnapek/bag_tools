#ifdef _MSC_VER
#include "memory_mapped_file.h"
#else
#include "read_only_file.h"
#include "read_only_file_mapping.h"
#endif

#include "bag.h"
#include "overload.h"
#include "scope_exit.h"
#include "utils.h"

#include <algorithm> // std::find, std::all_of, std::any_of
#include <array>
#include <cassert>
#include <cinttypes>
#include <cstdint> // std::uint64_t, std::uint32_t
#include <cstdio> // std::puts
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memcmp, std::memcpy
#include <iterator> // std::size, std::cbegin, std::cend
#include <utility> // std::move
#include <vector>


#ifdef _MSC_VER
typedef wchar_t native_char_t;
#define main_function wmain
#else
typedef char native_char_t;
#define main_function main
#endif


bool bag_to_pcap(int const argc, native_char_t const* const* const argv);
bool bag_to_pcap(native_char_t const* const& input_file_name, native_char_t const* const& output_file_name);
void print_records(mk::bag::records_t const& records);


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
	CHECK_RET(argc == 3, false);
	bool const bussiness = bag_to_pcap(argv[1], argv[2]);
	CHECK_RET(bussiness, false);
	return true;
}

bool bag_to_pcap(native_char_t const* const& input_file_name, native_char_t const* const& output_file_name)
{
	#ifdef _MSC_VER
	memory_mapped_file const input_file{input_file_name};
	CHECK_RET(input_file.begin() != nullptr, false);
	unsigned char const* const input_file_ptr = reinterpret_cast<unsigned char const*>(input_file.begin());
	std::uint64_t const input_file_size = input_file.size();
	#else
	mk::read_only_file_t const input_file{input_file_name};
	CHECK_RET(input_file, false);
	mk::read_only_file_mapping_t const input_file_mapping{input_file};
	CHECK_RET(input_file_mapping, false);
	unsigned char const* const input_file_ptr = static_cast<unsigned char const*>(input_file_mapping.get());
	std::uint64_t const input_file_size = input_file_mapping.get_size();
	#endif

	mk::bag::records_t records;
	CHECK_RET(mk::bag::parse(input_file_ptr, input_file_size, &records), false);

	print_records(records);

	(void)output_file_name;

	return true;
}


char const* get_record_type(mk::bag::record_t const& record)
{
	return std::visit(mk::make_overload
	(
		[](mk::bag::header::bag_t const&) -> char const* { return "bag"; },
		[](mk::bag::header::chunk_t const&) -> char const* { return "chunk"; },
		[](mk::bag::header::connection_t const&) -> char const* { return "connection"; },
		[](mk::bag::header::message_data_t const&) -> char const* { return "message_data"; },
		[](mk::bag::header::index_data_t const&) -> char const* { return "index_data"; },
		[](mk::bag::header::chunk_info_t const&) -> char const* { return "chunk_info"; }
	), record.m_header);
}

char const* get_record_details(mk::bag::record_t const& record, std::array<char, 256>& buff)
{
	std::visit(mk::make_overload
	(
		[&](mk::bag::header::bag_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "index_pos = %" PRIu64 ", conn_count = %" PRIu32 ", chunk_count = %" PRIu32 "", obj.m_index_pos, obj.m_conn_count, obj.m_chunk_count); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); },
		[&](mk::bag::header::chunk_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "compression = %.*s, size = %" PRIu32 "", obj.m_compression.m_len, obj.m_compression.m_begin, obj.m_size); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); },
		[&](mk::bag::header::connection_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "conn = %" PRIu32 ", topic = %.*s", obj.m_conn, obj.m_topic.m_len, obj.m_topic.m_begin); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); },
		[&](mk::bag::header::message_data_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "conn = %" PRIu32 ", time = %" PRIu64 "", obj.m_conn, obj.m_time); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); },
		[&](mk::bag::header::index_data_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "ver = %" PRIu32 ", conn = %" PRIu32 ", count = %" PRIu32 "", obj.m_ver, obj.m_conn, obj.m_count); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); },
		[&](mk::bag::header::chunk_info_t const& obj) -> void { int const formatted = std::snprintf(buff.data(), buff.size(), "ver = %" PRIu32 ", chunk_pos = %" PRIu64 ", start_time = %" PRIu64 ", end_time = %" PRIu64 ", count = %" PRIu32 "", obj.m_ver, obj.m_chunk_pos, obj.m_start_time, obj.m_end_time, obj.m_count); CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size())); }
	), record.m_header);
	return buff.data();
}

void print_records(mk::bag::records_t const& records)
{
	int i = 0;
	for(mk::bag::record_t const& record : records)
	{
		std::array<char, 256> buff;
		std::array<char, 256> buff_details;
		int const formatted = std::snprintf(buff.data(), buff.size(), "Record #%d, type = %s, size = %d, %s.\n", i, get_record_type(record), record.m_data.m_len, get_record_details(record, buff_details));
		CHECK_RET_V(formatted >= 0 && formatted < static_cast<int>(buff.size()));
		std::printf("%s", buff.data());
		++i;
	}
}
