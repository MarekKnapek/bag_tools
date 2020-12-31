#include "bag_tool_info_impl.h"

#include "data_source_mem.h"
#include "data_source_rommf.h"
#include "overload.h"
#include "read_only_memory_mapped_file.h"
#include "utils.h"

#include <cassert>
#include <cinttypes> // PRIu32, PRIu64
#include <cstdio>


mk::bag_tool::detail::bag_info_t::bag_info_t() :
	m_counter(),
	m_bag_hdr()
{
}


bool mk::bag_tool::detail::bag_info(int const argc, native_char_t const* const* const argv)
{
	CHECK_RET_F(argc == 3);
	return bag_info(argv[2]);
}


bool mk::bag_tool::detail::bag_info(native_char_t const* const input_bag)
{
	mk::read_only_memory_mapped_file_t const rommf{input_bag};
	if(rommf)
	{
		mk::data_source_mem_t data_source_mem = mk::data_source_mem_t::make(rommf.get_data(), static_cast<std::size_t>(rommf.get_size()));
		CHECK_RET_F(data_source_mem);
		bool const processed = bag_info(data_source_mem);
		CHECK_RET_F(processed);
		return true;
	}
	else
	{
		mk::data_source_rommf_t data_source_rommf = mk::data_source_rommf_t::make(input_bag);
		CHECK_RET_F(data_source_rommf);
		bool const processed = bag_info(data_source_rommf);
		CHECK_RET_F(processed);
		return true;
	}
}

template<typename data_source_t>
bool mk::bag_tool::detail::bag_info(data_source_t& data_source)
{
	CHECK_RET_F(mk::bag::is_bag_file(data_source));
	data_source.consume(mk::bag::bag_file_header_len());

	static constexpr auto const s_record_callback = [](void* const ctx, void* const data) -> bool
	{
		assert(ctx);
		assert(data);

		bag_info_t& bag_info = *static_cast<bag_info_t*>(ctx);
		mk::bag::record_t const& record = *static_cast<mk::bag::record_t const*>(data);

		bool const record_processed = process_record(bag_info, record);
		CHECK_RET_F(record_processed);

		return true;
	};
	mk::bag::callback_t const callback = s_record_callback;

	bag_info_t bag_info;
	bool const records_parsed = mk::bag::parse_records(data_source, callback, &bag_info);

	return true;
}

bool mk::bag_tool::detail::process_record(bag_info_t& bag_info, mk::bag::record_t const& record)
{
	std::uint32_t const counter = bag_info.m_counter;
	char const* const type_name = get_record_type_name(record);

	std::printf("%u, %s, ", counter, type_name);

	bool const type_processed = std::visit
	(
		make_overload
		(
			[&](mk::bag::header::bag_t const& header) -> bool { return process_type(bag_info, record, header); },
			[&](mk::bag::header::chunk_t const& header) -> bool { return process_type(bag_info, record, header); },
			[&](mk::bag::header::connection_t const& header) -> bool { return process_type(bag_info, record, header); },
			[&](mk::bag::header::message_data_t const& header) -> bool { return process_type(bag_info, record, header); },
			[&](mk::bag::header::index_data_t const& header) -> bool { return process_type(bag_info, record, header); },
			[&](mk::bag::header::chunk_info_t const& header) -> bool { return process_type(bag_info, record, header); }
		),
		record.m_header
	);
	CHECK_RET_F(type_processed);

	std::printf(", data size = %d\n" , record.m_data.m_len);

	++bag_info.m_counter;

	return true;
}

char const* mk::bag_tool::detail::get_record_type_name(mk::bag::record_t const& record)
{
	char const* const record_type_name = std::visit
	(
		make_overload
		(
			[](mk::bag::header::bag_t const&) -> char const* { return "bag"; },
			[](mk::bag::header::chunk_t const&) -> char const* { return "chunk"; },
			[](mk::bag::header::connection_t const&) -> char const* { return "connection"; },
			[](mk::bag::header::message_data_t const&) -> char const* { return "message_data"; },
			[](mk::bag::header::index_data_t const&) -> char const* { return "index_data"; },
			[](mk::bag::header::chunk_info_t const&) -> char const* { return "chunk_info"; }
		),
		record.m_header
	);
	return record_type_name;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::bag_t const& /* tag */)
{
	mk::bag::header::bag_t const& header = std::get<mk::bag::header::bag_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter == 0);

	bag_info.m_bag_hdr = header;

	std::printf
	(
		"index_pos = %" PRIu64 ", conn_count = %" PRIu32 ", chunk_count = %" PRIu32,
		header.m_index_pos,
		header.m_conn_count,
		header.m_chunk_count
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::chunk_t const& /* tag */)
{
	mk::bag::header::chunk_t const& header = std::get<mk::bag::header::chunk_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter != 0);

	std::printf
	(
		"compression = %.*s, size = %" PRIu32,
		header.m_compression.m_len,
		header.m_compression.m_begin,
		header.m_size
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::connection_t const& /* tag */)
{
	mk::bag::header::connection_t const& header = std::get<mk::bag::header::connection_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter != 0);

	std::printf
	(
		"m_conn = %" PRIu32 ", m_topic = %.*s",
		header.m_conn,
		header.m_topic.m_len,
		header.m_topic.m_begin
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::message_data_t const& /* tag */)
{
	mk::bag::header::message_data_t const& header = std::get<mk::bag::header::message_data_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter != 0);

	std::printf
	(
		"conn = %" PRIu32 ", time = %" PRIu64,
		header.m_conn,
		header.m_time
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::index_data_t const& /* tag */)
{
	mk::bag::header::index_data_t const& header = std::get<mk::bag::header::index_data_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter != 0);

	std::printf
	(
		"ver = %" PRIu32 ", conn = %" PRIu32 ", count = %" PRIu32,
		header.m_ver,
		header.m_conn,
		header.m_count
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::chunk_info_t const& /* tag */)
{
	mk::bag::header::chunk_info_t const& header = std::get<mk::bag::header::chunk_info_t>(record.m_header);
	CHECK_RET_F(bag_info.m_counter != 0);

	std::printf
	(
		"ver = %" PRIu32 ", chunk_pos = %" PRIu64 ", start_time = %" PRIu64 ", end_time = %" PRIu64 ", count = %" PRIu32,
		header.m_ver,
		header.m_chunk_pos,
		header.m_start_time,
		header.m_end_time,
		header.m_count
	);

	return true;
}
