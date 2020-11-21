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
bool print_records(mk::bag::records_t const& records);
bool find_os_lidar_packets(mk::bag::records_t const& records, std::uint32_t* const& out_os_lidar_packets_connection);


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

	bool const printed = print_records(records);
	CHECK_RET(printed, false);

	std::uint32_t os_lidar_packets_connection;
	bool const os_lidar_packets_found = find_os_lidar_packets(records, &os_lidar_packets_connection);
	CHECK_RET(os_lidar_packets_found, false);

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

bool get_record_details(mk::bag::record_t const& record, std::array<char, 256>& buff)
{
	bool const formatted = std::visit(mk::make_overload
	(
		[&](mk::bag::header::bag_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "index_pos = %" PRIu64 ", conn_count = %" PRIu32 ", chunk_count = %" PRIu32 "", obj.m_index_pos, obj.m_conn_count, obj.m_chunk_count); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; },
		[&](mk::bag::header::chunk_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "compression = %.*s, size = %" PRIu32 "", obj.m_compression.m_len, obj.m_compression.m_begin, obj.m_size); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; },
		[&](mk::bag::header::connection_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "conn = %" PRIu32 ", topic = %.*s", obj.m_conn, obj.m_topic.m_len, obj.m_topic.m_begin); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; },
		[&](mk::bag::header::message_data_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "conn = %" PRIu32 ", time = %" PRIu64 "", obj.m_conn, obj.m_time); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; },
		[&](mk::bag::header::index_data_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "ver = %" PRIu32 ", conn = %" PRIu32 ", count = %" PRIu32 "", obj.m_ver, obj.m_conn, obj.m_count); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; },
		[&](mk::bag::header::chunk_info_t const& obj) -> bool { int const formatted = std::snprintf(buff.data(), buff.size(), "ver = %" PRIu32 ", chunk_pos = %" PRIu64 ", start_time = %" PRIu64 ", end_time = %" PRIu64 ", count = %" PRIu32 "", obj.m_ver, obj.m_chunk_pos, obj.m_start_time, obj.m_end_time, obj.m_count); CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false); return true; }
	), record.m_header);
	CHECK_RET(formatted, false);
	return true;
}

bool print_records(mk::bag::records_t const& records)
{
	int i = 0;
	for(mk::bag::record_t const& record : records)
	{
		std::array<char, 256> buff;
		std::array<char, 256> buff_details;
		bool const got_details = get_record_details(record, buff_details);
		CHECK_RET(got_details, false);
		int const formatted = std::snprintf(buff.data(), buff.size(), "Record #%d, type = %s, size = %d, %s.\n", i, get_record_type(record), record.m_data.m_len, buff_details.data());
		CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false);
		std::printf("%s", buff.data());
		++i;
	}
	return true;
}

namespace mk{namespace bag{namespace detail{
	bool parse_fields(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint32_t const& header_len, fields_t* const& out_fields, int* const& out_fields_count);
	bool parse_connection_data(field_t const* const& fields, int const& fields_count, data::connection_data_t* const& out_connection_data);
}}}

bool find_os_lidar_packets(mk::bag::records_t const& records, std::uint32_t* const& out_os_lidar_packets_connection)
{
	static constexpr char const s_os_lidar_packets_connection_topic_name[] = "/os_node/lidar_packets";
	static constexpr int const s_os_lidar_packets_connection_topic_name_len = static_cast<int>(std::size(s_os_lidar_packets_connection_topic_name)) - 1;

	assert(out_os_lidar_packets_connection);
	std::uint32_t& os_lidar_packets_connection = *out_os_lidar_packets_connection;
	for(mk::bag::record_t const& record : records)
	{
		bool const is_connection = std::visit(mk::make_overload([](mk::bag::header::connection_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
		if(!is_connection) continue;
		mk::bag::header::connection_t const& connection = std::get<mk::bag::header::connection_t>(record.m_header);
		bool const is_os_lidar_packets_connection = connection.m_topic.m_len == s_os_lidar_packets_connection_topic_name_len && std::memcmp(connection.m_topic.m_begin, s_os_lidar_packets_connection_topic_name, s_os_lidar_packets_connection_topic_name_len) == 0;
		if(!is_os_lidar_packets_connection) continue;
		os_lidar_packets_connection = connection.m_conn;

		std::uint64_t idx = 0;
		mk::bag::fields_t os_lidar_packets_connection_fields;
		int os_lidar_packets_connection_fields_count;
		bool const fields_parsed = mk::bag::detail::parse_fields(record.m_data.m_begin, record.m_data.m_len, idx, record.m_data.m_len, &os_lidar_packets_connection_fields, &os_lidar_packets_connection_fields_count);
		CHECK_RET(fields_parsed, false);

		mk::bag::data::connection_data_t connection_data;
		bool const connection_data_parsed = mk::bag::detail::parse_connection_data(os_lidar_packets_connection_fields.data(), os_lidar_packets_connection_fields_count, &connection_data);
		CHECK_RET(connection_data_parsed, false);

		return true;
	}
	return false;
}
