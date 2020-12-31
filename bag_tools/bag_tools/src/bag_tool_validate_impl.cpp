#include "bag_tool_validate_impl.h"

#include "data_source_mem.h"
#include "data_source_rommf.h"
#include "overload.h"
#include "read_only_memory_mapped_file.h"
#include "utils.h"

#include <cassert>
#include <cinttypes> // PRIu32, PRIu64
#include <cstdio>


mk::bag_tool::detail::bag_validate_t::bag_validate_t() :
	m_data_source(),
	m_data_source_type(),
	m_counter(),
	m_bag_hdr(),
	m_connection_automat(connection_automat_e::before)
{
}


bool mk::bag_tool::detail::bag_validate(int const argc, native_char_t const* const* const argv)
{
	CHECK_RET_F(argc == 3);
	return bag_validate(argv[2]);
}


bool mk::bag_tool::detail::bag_validate(native_char_t const* const input_bag)
{
	bag_validate_t bag_validate;
	mk::read_only_memory_mapped_file_t const rommf{input_bag};
	if(rommf)
	{
		mk::data_source_mem_t data_source_mem = mk::data_source_mem_t::make(rommf.get_data(), static_cast<std::size_t>(rommf.get_size()));
		CHECK_RET_F(data_source_mem);
		bag_validate.m_data_source = &data_source_mem;
		bag_validate.m_data_source_type = 1;
		bool const processed = detail::bag_validate(bag_validate, data_source_mem); // TODO: Why this doesn't work without namespace?
		CHECK_RET_F(processed);
		return true;
	}
	else
	{
		mk::data_source_rommf_t data_source_rommf = mk::data_source_rommf_t::make(input_bag);
		CHECK_RET_F(data_source_rommf);
		bag_validate.m_data_source = &data_source_rommf;
		bag_validate.m_data_source_type = 2;
		bool const processed = detail::bag_validate(bag_validate, data_source_rommf); // TODO: Why this doesn't work without namespace?
		CHECK_RET_F(processed);
		return true;
	}
}

template<typename data_source_t>
bool mk::bag_tool::detail::bag_validate(bag_validate_t& bag_validate, data_source_t& data_source)
{
	CHECK_RET_F(mk::bag::is_bag_file(data_source));
	data_source.consume(mk::bag::bag_file_header_len());

	static constexpr auto const s_record_callback = [](void* const ctx, void* const data, [[maybe_unused]] bool& keep_iterating) -> bool
	{
		assert(ctx);
		assert(data);

		bag_validate_t& bag_validate = *static_cast<bag_validate_t*>(ctx);
		mk::bag::record_t const& record = *static_cast<mk::bag::record_t const*>(data);

		bool const record_processed = process_record(bag_validate, record);
		CHECK_RET_F(record_processed);

		return true;
	};
	mk::bag::callback_t const callback = s_record_callback;

	bool const records_parsed = mk::bag::parse_records(data_source, callback, &bag_validate);

	return true;
}

bool mk::bag_tool::detail::process_record(bag_validate_t& bag_validate, mk::bag::record_t const& record)
{
	std::printf("%u, ", bag_validate.m_counter);

	bool const type_processed = std::visit
	(
		make_overload
		(
			[&](mk::bag::header::bag_t const& header) -> bool { return process_type(bag_validate, record, header); },
			[&](mk::bag::header::chunk_t const& header) -> bool { return process_type(bag_validate, record, header); },
			[&](mk::bag::header::connection_t const& header) -> bool { return process_type(bag_validate, record, header); },
			[&](mk::bag::header::message_data_t const& header) -> bool { return process_type(bag_validate, record, header); },
			[&](mk::bag::header::index_data_t const& header) -> bool { return process_type(bag_validate, record, header); },
			[&](mk::bag::header::chunk_info_t const& header) -> bool { return process_type(bag_validate, record, header); }
		),
		record.m_header
	);
	CHECK_RET_F(type_processed);

	std::printf(", data size = %d\n" , record.m_data.m_len);

	++bag_validate.m_counter;

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::bag_t const& /* tag */)
{
	mk::bag::header::bag_t const& header = std::get<mk::bag::header::bag_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter == 0);

	bag_validate.m_bag_hdr = header;

	std::printf
	(
		"index_pos = %" PRIu64 ", conn_count = %" PRIu32 ", chunk_count = %" PRIu32,
		header.m_index_pos,
		header.m_conn_count,
		header.m_chunk_count
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::chunk_t const& /* tag */)
{
	mk::bag::header::chunk_t const& header = std::get<mk::bag::header::chunk_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter != 0);
	CHECK_RET_F(bag_validate.m_connection_automat == connection_automat_e::before);

	std::printf
	(
		"compression = %.*s, size = %" PRIu32,
		header.m_compression.m_len,
		header.m_compression.m_begin,
		header.m_size
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::connection_t const& /* tag */)
{
	mk::bag::header::connection_t const& header = std::get<mk::bag::header::connection_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter != 0);

	std::printf
	(
		"m_conn = %" PRIu32 ", m_topic = %.*s",
		header.m_conn,
		header.m_topic.m_len,
		header.m_topic.m_begin
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::message_data_t const& /* tag */)
{
	mk::bag::header::message_data_t const& header = std::get<mk::bag::header::message_data_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter != 0);

	std::printf
	(
		"conn = %" PRIu32 ", time = %" PRIu64,
		header.m_conn,
		header.m_time
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::index_data_t const& /* tag */)
{
	mk::bag::header::index_data_t const& header = std::get<mk::bag::header::index_data_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter != 0);

	std::printf
	(
		"ver = %" PRIu32 ", conn = %" PRIu32 ", count = %" PRIu32,
		header.m_ver,
		header.m_conn,
		header.m_count
	);

	return true;
}

bool mk::bag_tool::detail::process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::chunk_info_t const& /* tag */)
{
	mk::bag::header::chunk_info_t const& header = std::get<mk::bag::header::chunk_info_t>(record.m_header);
	CHECK_RET_F(bag_validate.m_counter != 0);

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
