#include "read_only_memory_mapped_file.h"
#include "bag.h"
#include "overload.h"
#include "scope_exit.h"
#include "utils.h"
#include "cross_platform.h"

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

#include <lz4frame.h>


bool bag_to_pcap(int const argc, native_char_t const* const* const argv);
bool bag_to_pcap(native_char_t const* const& input_file_name, native_char_t const* const& output_file_name);
bool get_os_channel(mk::read_only_memory_mapped_file_t const& rommf, std::uint32_t* const& out_os_channel);
bool process_record_os_channel(mk::bag::record_t const& record, std::optional<std::uint32_t>& os_channel_opt);
bool filter_os_node_lidar_packets_connection_topic(mk::bag::record_t const& record, bool* const& out_satisfies);

/*bool find_os_lidar_packets(mk::bag::records_t const& records, std::uint32_t* const& out_os_lidar_packets_connection);
bool process_os_lidar_packets_connection(mk::bag::records_t const& records, std::uint32_t const& os_lidar_packets_connection);*/


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
	mk::read_only_memory_mapped_file_t const input_file{input_file_name};
	CHECK_RET(input_file, false);

	std::uint32_t os_channel;
	bool const got_os_channel = get_os_channel(input_file, &os_channel);
	CHECK_RET(got_os_channel, false);

	/*std::uint32_t os_lidar_packets_connection;
	bool const os_lidar_packets_found = find_os_lidar_packets(records, &os_lidar_packets_connection);
	CHECK_RET(os_lidar_packets_found, false);

	bool const processed = process_os_lidar_packets_connection(records, os_lidar_packets_connection);
	CHECK_RET(processed, false);*/

	(void)output_file_name;

	return true;
}

bool get_os_channel(mk::read_only_memory_mapped_file_t const& rommf, std::uint32_t* const& out_os_channel)
{
	assert(out_os_channel);
	std::uint32_t& os_channel = *out_os_channel;

	unsigned char const* const input_file_ptr = static_cast<unsigned char const*>(rommf.get_data());
	std::uint64_t const input_file_size = rommf.get_size();

	std::optional<std::uint32_t> os_channel_opt = std::nullopt;
	static constexpr auto const s_record_callback = [](void* const& ctx, mk::bag::callback_variant_e const& variant, void const* const& data) -> bool
	{
		std::optional<std::uint32_t>& os_channel_opt = *static_cast<std::optional<std::uint32_t>*>(ctx);
		CHECK_RET(variant == mk::bag::callback_variant_e::record, false);
		mk::bag::record_t const& record = *static_cast<mk::bag::record_t const*>(data);

		bool const processed = process_record_os_channel(record, os_channel_opt);
		CHECK_RET(processed, false);

		return true;
	};
	bool const file_parsed = mk::bag::parse_file(input_file_ptr, input_file_size, s_record_callback, &os_channel_opt);
	CHECK_RET(file_parsed, false);

	CHECK_RET(os_channel_opt.has_value(), false);
	os_channel = *os_channel_opt;

	return true;
}

bool process_record_os_channel(mk::bag::record_t const& record, std::optional<std::uint32_t>& os_channel_opt)
{
	bool is_os_node_lidar_packets_connection_topic;
	bool const filtered = filter_os_node_lidar_packets_connection_topic(record, &is_os_node_lidar_packets_connection_topic);
	CHECK_RET(filtered, false);
	if(!is_os_node_lidar_packets_connection_topic) return true;

	/*struct fields_callback_ctx_t
	{
		mk::bag::fields_t& m_fields;
		int& m_fields_count;
	};
	static constexpr auto const s_field_callback = [](void* const& ctx, mk::bag::callback_variant_e const& variant, void const* const& data) -> bool
	{
		fields_callback_ctx_t& fields_callback_ctx = *static_cast<fields_callback_ctx_t*>(ctx);
		CHECK_RET(variant == mk::bag::callback_variant_e::field, false);
		mk::bag::field_t const& field = *static_cast<mk::bag::field_t const*>(data);

		CHECK_RET(fields_callback_ctx.m_fields_count < mk::bag::s_fields_max, false);
		fields_callback_ctx.m_fields[fields_callback_ctx.m_fields_count] = field;
		++fields_callback_ctx.m_fields_count;

		return true;
	};

	std::uint64_t idx = 0;
	mk::bag::fields_t fields;
	int fields_count = 0;
	fields_callback_ctx_t fields_callback_ctx{fields, fields_count};
	bool const fields_parsed = mk::bag::parse_fields(record.m_data.m_begin, record.m_data.m_len, idx, record.m_data.m_len, s_field_callback, &fields_callback_ctx);
	CHECK_RET(fields_parsed, false);

	mk::bag::data::connection_data_t connection_data;
	bool const connection_data_parsed = mk::bag::parse_connection_data(fields.data(), fields_count, &connection_data);
	CHECK_RET(connection_data_parsed, false);*/

	CHECK_RET(!os_channel_opt.has_value(), false);
	os_channel_opt = std::get<mk::bag::header::connection_t>(record.m_header).m_conn;

	return true;
}

bool filter_os_node_lidar_packets_connection_topic(mk::bag::record_t const& record, bool* const& out_satisfies)
{
	static constexpr char const s_os_node_lidar_packets_connection_topic_name[] = "/os_node/lidar_packets";
	static constexpr int const s_os_node_lidar_packets_connection_topic_name_len = static_cast<int>(std::size(s_os_node_lidar_packets_connection_topic_name)) - 1;

	assert(out_satisfies);
	bool& satisfies = *out_satisfies;

	bool const is_connection = std::visit(mk::make_overload([](mk::bag::header::connection_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
	if(!is_connection)
	{
		satisfies = false;
		return true;
	}
	mk::bag::header::connection_t const& connection = std::get<mk::bag::header::connection_t>(record.m_header);

	bool const is_os_node_lidar_packets_connection_topic = connection.m_topic.m_len == s_os_node_lidar_packets_connection_topic_name_len && std::memcmp(connection.m_topic.m_begin, s_os_node_lidar_packets_connection_topic_name, s_os_node_lidar_packets_connection_topic_name_len) == 0;
	if(!is_os_node_lidar_packets_connection_topic)
	{
		satisfies = false;
		return true;
	}

	satisfies = true;
	return true;
}




/*namespace mk{namespace bag{namespace detail{
	bool parse_fields(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint32_t const& header_len, fields_t* const& out_fields, int* const& out_fields_count);
	bool parse_connection_data(field_t const* const& fields, int const& fields_count, data::connection_data_t* const& out_connection_data);
	bool parse_record(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, record_t* const& out_record);
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

bool decompress(void const* const& input, int const& input_len, void* const& output, int const& output_len)
{
	LZ4F_decompressionContext_t ctx;
	LZ4F_errorCode_t const context_created = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
	CHECK_RET(!LZ4F_isError(context_created), false);
	auto const context_free = mk::make_scope_exit([&](){ LZ4F_errorCode_t const context_freed = LZ4F_freeDecompressionContext(ctx); CHECK_RET_V(context_freed == 0); });

	std::size_t destination_size = output_len;
	std::size_t source_size = input_len;
	std::size_t const decompressed = LZ4F_decompress(ctx, output, &destination_size, input, &source_size, nullptr);
	CHECK_RET(decompressed == 0 && destination_size == static_cast<std::size_t>(output_len) && source_size == static_cast<std::size_t>(input_len), false);

	return true;
}

bool process_os_lidar_packet_connection(mk::bag::record_t const& record, std::uint32_t const& os_lidar_packets_connection)
{
	static constexpr char const s_compression_lz4_name[] = "lz4";
	static constexpr int const s_compression_lz4_name_len = static_cast<int>(std::size(s_compression_lz4_name)) - 1;

	bool const is_chunk = std::visit(mk::make_overload([](mk::bag::header::chunk_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
	if(!is_chunk) return true;
	mk::bag::header::chunk_t const& chunk = std::get<mk::bag::header::chunk_t>(record.m_header);

	bool const is_lz4 = chunk.m_compression.m_len == s_compression_lz4_name_len && std::memcmp(chunk.m_compression.m_begin, s_compression_lz4_name, s_compression_lz4_name_len) == 0;
	CHECK_RET(is_lz4, false);

	std::vector<unsigned char> uncompressed_data;
	uncompressed_data.resize(chunk.m_size);
	bool const decompressed = decompress(record.m_data.m_begin, record.m_data.m_len, uncompressed_data.data(), static_cast<int>(uncompressed_data.size()));
	CHECK_RET(decompressed, false);

	std::uint64_t idx = 0;
	for(;;)
	{
		mk::bag::record_t new_record;
		bool const parsed = mk::bag::detail::parse_record(uncompressed_data.data(), uncompressed_data.size(), idx, &new_record);
		CHECK_RET(parsed, false);
	}

	return true;
}

bool process_os_lidar_packets_connection(mk::bag::records_t const& records, std::uint32_t const& os_lidar_packets_connection)
{
	for(mk::bag::record_t const& record : records)
	{
		bool const processed = process_os_lidar_packet_connection(record, os_lidar_packets_connection);
		CHECK_RET(processed, false);
	}
	return true;
}*/
