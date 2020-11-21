#include "bag_print_file.h"

#include "bag.h"
#include "utils.h"
#include "overload.h"
#include "read_only_memory_mapped_file.h"

#include <array>
#include <cstdio> // std::printf, std::snprintf
#include <cinttypes> // PRIu32, PRIu64


namespace mk
{
	namespace bag
	{
		namespace detail
		{
			bool print_record(record_t const& record, int& record_idx);
			char const* get_record_type(mk::bag::record_t const& record);
			bool get_record_details(mk::bag::record_t const& record, char* const& buff, int const& buff_len);
		}
	}
}


bool mk::bag::print_file(native_char_t const* const& file_path)
{
	static constexpr auto const s_record_callback = [](void* const& ctx, callback_variant_e const& variant, void const* const& data) -> bool
	{
		int& record_idx = *static_cast<int*>(ctx);
		CHECK_RET(variant == callback_variant_e::record, false);
		record_t const& record = *static_cast<record_t const*>(data);

		bool const printed = detail::print_record(record, record_idx);
		CHECK_RET(printed, false);

		return true;
	};

	read_only_memory_mapped_file_native_t const file{file_path};
	CHECK_RET(file, false);

	int record_idx = 0;
	bool const parsed = parse_file(file.get_data(), file.get_size(), s_record_callback, &record_idx);
	CHECK_RET(parsed, false);

	return true;
}


bool mk::bag::detail::print_record(record_t const& record, int& record_idx)
{
	static constexpr int const s_max_record_idx = 1 * 1024 * 1024 * 1024;
	CHECK_RET(record_idx < s_max_record_idx, false);

	std::array<char, 256> buff;
	std::array<char, 256> buff_details;
	char const* const record_type = get_record_type(record);
	bool const got_details = get_record_details(record, buff_details.data(), static_cast<int>(buff_details.size()));
	CHECK_RET(got_details, false);
	int const formatted = std::snprintf(buff.data(), buff.size(), "Record #%d, type = %s, size = %d, %s.\n", record_idx, record_type, record.m_data.m_len, buff_details.data());
	CHECK_RET(formatted >= 0 && formatted < static_cast<int>(buff.size()), false);
	std::printf("%s", buff.data());

	++record_idx;
	return true;
}

char const* mk::bag::detail::get_record_type(record_t const& record)
{
	return std::visit
	(
		make_overload
		(
			[](header::bag_t const&) -> char const* { return "bag"; },
			[](header::chunk_t const&) -> char const* { return "chunk"; },
			[](header::connection_t const&) -> char const* { return "connection"; },
			[](header::message_data_t const&) -> char const* { return "message_data"; },
			[](header::index_data_t const&) -> char const* { return "index_data"; },
			[](header::chunk_info_t const&) -> char const* { return "chunk_info"; }
		),
		record.m_header
	);
}

bool mk::bag::detail::get_record_details(record_t const& record, char* const& buff, int const& buff_len)
{
	bool const formatted = std::visit
	(
		mk::make_overload
		(
			[&](header::bag_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "index_pos = %" PRIu64 ", conn_count = %" PRIu32 ", chunk_count = %" PRIu32 "", obj.m_index_pos, obj.m_conn_count, obj.m_chunk_count); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; },
			[&](header::chunk_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "compression = %.*s, size = %" PRIu32 "", obj.m_compression.m_len, obj.m_compression.m_begin, obj.m_size); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; },
			[&](header::connection_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "conn = %" PRIu32 ", topic = %.*s", obj.m_conn, obj.m_topic.m_len, obj.m_topic.m_begin); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; },
			[&](header::message_data_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "conn = %" PRIu32 ", time = %" PRIu64 "", obj.m_conn, obj.m_time); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; },
			[&](header::index_data_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "ver = %" PRIu32 ", conn = %" PRIu32 ", count = %" PRIu32 "", obj.m_ver, obj.m_conn, obj.m_count); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; },
			[&](header::chunk_info_t const& obj) -> bool { int const formatted = std::snprintf(buff, buff_len, "ver = %" PRIu32 ", chunk_pos = %" PRIu64 ", start_time = %" PRIu64 ", end_time = %" PRIu64 ", count = %" PRIu32 "", obj.m_ver, obj.m_chunk_pos, obj.m_start_time, obj.m_end_time, obj.m_count); CHECK_RET(formatted >= 0 && formatted < buff_len, false); return true; }
		),
		record.m_header
	);
	CHECK_RET(formatted, false);
	return true;
}
