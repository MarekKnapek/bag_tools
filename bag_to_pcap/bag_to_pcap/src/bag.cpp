#include "bag.h"

#include "overload.h"
#include "utils.h" // CHECK_RET

#include <algorithm> // std::any_of, std::find, std::find_if
#include <array>
#include <cassert>
#include <cstring> // std::memcmp
#include <iterator> // std::size, std::cbegin, std::cend


namespace mk
{
	namespace bag
	{
		namespace detail
		{
			enum class field_descr_e
			{
				u32,
				u64,
				str,
			};
			struct field_descr_t
			{
				char const* m_name;
				int m_name_len;
				field_descr_e m_type;
			};
			template<typename t> t read(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx);
			bool parse_field(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint_fast32_t const& header_len, std::uint32_t& header_idx, field_t* const& out_field);
			bool parse_field_descr(field_t const* const& fields, int const& fields_count, field_descr_t const& field_descr, void* const& target);
			bool parse_field_descrs(field_t const* const& fields, int const& fields_count, field_descr_t const* const& field_descrs, void* const* const& targets, int const& field_descrs_count);
			bool parse_bag(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_chunk(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_connection(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_message_data(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_index_data(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_chunk_info(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_header(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header);
			bool parse_header(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint32_t const& header_len, header::header_t* const& out_header);
			bool parse_record(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, record_t* const& out_record);
		}
	}
}


template<typename t> t mk::bag::detail::read(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx)
{
	t val;
	assert(len - idx >= sizeof(val)); (void)len;
	std::memcpy(&val, data + idx, sizeof(val));
	idx += sizeof(val);
	return val;
}

bool mk::bag::detail::parse_field(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint_fast32_t const& header_len, std::uint32_t& header_idx, field_t* const& out_field)
{
	static constexpr int const s_max_field_len = 1 * 1024 * 1024 * 1024;

	assert(out_field);
	field_t& field = *out_field;

	CHECK_RET(header_len - header_idx >= sizeof(std::uint32_t), false);
	std::uint32_t const field_len = read<std::uint32_t>(data, len, idx);
	header_idx += sizeof(std::uint32_t);
	CHECK_RET(field_len <= s_max_field_len, false);
	CHECK_RET(header_len - header_idx >= field_len, false);

	static constexpr unsigned char const s_field_name_value_separator = static_cast<unsigned char>('=');
	auto const it = std::find(data + idx, data + idx + field_len, s_field_name_value_separator);
	CHECK_RET(it != data + idx + field_len, false);

	field.m_name.m_begin = reinterpret_cast<char const*>(data) + idx;
	field.m_name.m_len = static_cast<int>((it) - (data + idx));
	field.m_value.m_begin = it + 1;
	field.m_value.m_len = static_cast<int>((data + idx + field_len) - (it + 1));

	idx += field_len;
	header_idx += field_len;

	return true;
}

bool mk::bag::detail::parse_field_descr(field_t const* const& fields, int const& fields_count, field_descr_t const& field_descr, void* const& target)
{
	auto const it = std::find_if(fields, fields + fields_count, [&](field_t const& field) -> bool { return field.m_name.m_len == field_descr.m_name_len && std::memcmp(field.m_name.m_begin, field_descr.m_name, field_descr.m_name_len) == 0; });
	CHECK_RET(it != fields + fields_count, false);
	field_t const& field = *it;
	switch(field_descr.m_type)
	{
		case field_descr_e::u32:
		{
			std::uint32_t& target_with_type = *static_cast<std::uint32_t*>(target);
			CHECK_RET(field.m_value.m_len == sizeof(target_with_type), false);
			std::memcpy(&target_with_type, field.m_value.m_begin, sizeof(target_with_type));
		}
		break;
		case field_descr_e::u64:
		{
			std::uint64_t& target_with_type = *static_cast<std::uint64_t*>(target);
			CHECK_RET(field.m_value.m_len == sizeof(target_with_type), false);
			std::memcpy(&target_with_type, field.m_value.m_begin, sizeof(target_with_type));
		}
		break;
		case field_descr_e::str:
		{
			string_t& target_with_type = *static_cast<string_t*>(target);
			target_with_type.m_begin = reinterpret_cast<char const*>(field.m_value.m_begin);
			target_with_type.m_len = field.m_value.m_len;
		}
		break;
	}
	return true;
}

bool mk::bag::detail::parse_field_descrs(field_t const* const& fields, int const& fields_count, field_descr_t const* const& field_descrs, void* const* const& targets, int const& field_descrs_count)
{
	for(int i = 0; i != field_descrs_count; ++i)
	{
		bool const parsed = parse_field_descr(fields, fields_count, field_descrs[i], targets[i]);
		CHECK_RET(parsed, false);
	}
	return true;
}

bool mk::bag::detail::parse_bag(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "index_pos";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "conn_count";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr char const s_field_2_name[] = "chunk_count";
	static constexpr int const s_field_2_name_len = static_cast<int>(std::size(s_field_2_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::u64},
		{s_field_1_name, s_field_1_name_len, field_descr_e::u32},
		{s_field_2_name, s_field_2_name_len, field_descr_e::u32},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::bag_t{};
	header::bag_t& obj = std::get<header::bag_t>(header);
	void* const targets[] = {&obj.m_index_pos, &obj.m_conn_count, &obj.m_chunk_count};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_chunk(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "compression";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "size";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::str},
		{s_field_1_name, s_field_1_name_len, field_descr_e::u32},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::chunk_t{};
	header::chunk_t& obj = std::get<header::chunk_t>(header);
	void* const targets[] = {&obj.m_compression, &obj.m_size};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_connection(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "conn";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "topic";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::u32},
		{s_field_1_name, s_field_1_name_len, field_descr_e::str},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::connection_t{};
	header::connection_t& obj = std::get<header::connection_t>(header);
	void* const targets[] = {&obj.m_conn, &obj.m_topic};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_message_data(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "conn";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "time";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::u32},
		{s_field_1_name, s_field_1_name_len, field_descr_e::u64},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::message_data_t{};
	header::message_data_t& obj = std::get<header::message_data_t>(header);
	void* const targets[] = {&obj.m_conn, &obj.m_time};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_index_data(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "ver";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "conn";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr char const s_field_2_name[] = "count";
	static constexpr int const s_field_2_name_len = static_cast<int>(std::size(s_field_2_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::u32},
		{s_field_1_name, s_field_1_name_len, field_descr_e::u32},
		{s_field_2_name, s_field_2_name_len, field_descr_e::u32},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::index_data_t{};
	header::index_data_t& obj = std::get<header::index_data_t>(header);
	void* const targets[] = {&obj.m_ver, &obj.m_conn, &obj.m_count};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_chunk_info(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr char const s_field_0_name[] = "ver";
	static constexpr int const s_field_0_name_len = static_cast<int>(std::size(s_field_0_name)) - 1;
	static constexpr char const s_field_1_name[] = "chunk_pos";
	static constexpr int const s_field_1_name_len = static_cast<int>(std::size(s_field_1_name)) - 1;
	static constexpr char const s_field_2_name[] = "start_time";
	static constexpr int const s_field_2_name_len = static_cast<int>(std::size(s_field_2_name)) - 1;
	static constexpr char const s_field_3_name[] = "end_time";
	static constexpr int const s_field_3_name_len = static_cast<int>(std::size(s_field_3_name)) - 1;
	static constexpr char const s_field_4_name[] = "count";
	static constexpr int const s_field_4_name_len = static_cast<int>(std::size(s_field_4_name)) - 1;
	static constexpr field_descr_t const s_field_descrs[] =
	{
		{s_field_0_name, s_field_0_name_len, field_descr_e::u32},
		{s_field_1_name, s_field_1_name_len, field_descr_e::u64},
		{s_field_2_name, s_field_2_name_len, field_descr_e::u64},
		{s_field_3_name, s_field_3_name_len, field_descr_e::u64},
		{s_field_4_name, s_field_4_name_len, field_descr_e::u32},
	};
	static constexpr int const s_field_descrs_count = static_cast<int>(std::size(s_field_descrs));

	assert(out_header);
	header::header_t& header = *out_header;

	header = header::chunk_info_t{};
	header::chunk_info_t& obj = std::get<header::chunk_info_t>(header);
	void* const targets[] = {&obj.m_ver, &obj.m_chunk_pos, &obj.m_start_time, &obj.m_end_time, &obj.m_count};
	bool const parsed = parse_field_descrs(fields, fields_count, s_field_descrs, targets, s_field_descrs_count);
	CHECK_RET(parsed, false);

	return true;
}

bool mk::bag::detail::parse_header(field_t const* const& fields, int const& fields_count, header::header_t* const& out_header)
{
	static constexpr int const s_min_fields_count = 2;
	static constexpr auto const s_find_op = [](field_t const& field) -> bool
	{
		static constexpr char const s_op_name[] = "op";
		static constexpr int const s_op_name_len = static_cast<int>(std::size(s_op_name)) - 1;
		static constexpr int const s_op_value_len = 1;

		return field.m_value.m_len == s_op_value_len && field.m_name.m_len == s_op_name_len && std::memcmp(field.m_name.m_begin, s_op_name, s_op_name_len) == 0;
	};
	static constexpr int const s_op_types[] = {header::bag_t::s_op, header::chunk_t::s_op, header::connection_t::s_op, header::message_data_t::s_op, header::index_data_t::s_op, header::chunk_info_t::s_op};

	assert(fields);
	assert(out_header);
	header::header_t& header = *out_header;

	CHECK_RET(fields_count >= s_min_fields_count, false);
	auto const it = std::find_if(fields, fields + fields_count, s_find_op);
	CHECK_RET(it != fields + fields_count, false);
	field_t const& op = *it;

	using std::cbegin;
	using std::cend;
	unsigned char const& op_type = op.m_value.m_begin[0];
	CHECK_RET(std::any_of(cbegin(s_op_types), cend(s_op_types), [&](auto const& e){ return e == op_type; }), false);
	switch(op_type)
	{
		case header::bag_t::s_op:
		{
			bool const parsed = parse_bag(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
		case header::chunk_t::s_op:
		{
			bool const parsed = parse_chunk(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
		case header::connection_t::s_op:
		{
			bool const parsed = parse_connection(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
		case header::message_data_t::s_op:
		{
			bool const parsed = parse_message_data(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
		case header::index_data_t::s_op:
		{
			bool const parsed = parse_index_data(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
		case header::chunk_info_t::s_op:
		{
			bool const parsed = parse_chunk_info(fields, fields_count, &header);
			CHECK_RET(parsed, false);
		}
		break;
	}

	return true;
}

bool mk::bag::detail::parse_header(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, std::uint32_t const& header_len, header::header_t* const& out_header)
{
	static constexpr int const s_max_fields = 8;

	assert(out_header);
	header::header_t& header = *out_header;

	std::array<field_t, s_max_fields> fields;
	int field_idx = 0;

	std::uint32_t header_idx = 0;
	while(header_idx != header_len)
	{
		CHECK_RET(field_idx < s_max_fields, false);
		field_t& field = fields[field_idx];
		++field_idx;
		bool const field_parsed = parse_field(data, len, idx, header_len, header_idx, &field);
		CHECK_RET(field_parsed, false);
	}

	bool const header_parsed = parse_header(fields.data(), field_idx, &header);
	CHECK_RET(header_parsed, false);

	return true;
}

bool mk::bag::detail::parse_record(unsigned char const* const& data, std::uint64_t const& len, std::uint64_t& idx, mk::bag::record_t* const& out_record)
{
	assert(out_record);
	record_t& record = *out_record;

	CHECK_RET(len - idx >= sizeof(std::uint32_t), false);
	std::uint32_t const header_len = read<std::uint32_t>(data, len, idx);
	CHECK_RET(len - idx >= header_len, false);
	bool const header_parsed = parse_header(data, len, idx, header_len, &record.m_header);
	CHECK_RET(header_parsed, false);

	CHECK_RET(len - idx >= sizeof(std::uint32_t), false);
	std::uint32_t const data_len = read<std::uint32_t>(data, len, idx);
	CHECK_RET(len - idx >= data_len, false);

	std::visit(make_overload
	(
		[&](header::index_data_t const& obj) -> void { CHECK_RET_V(data_len == obj.m_count * (sizeof(std::uint64_t) + sizeof(std::uint32_t))); },
		[&](header::chunk_info_t const& obj) -> void { CHECK_RET_V(data_len == obj.m_count * (sizeof(std::uint32_t) + sizeof(std::uint32_t))); },
		[&](...) -> void {}
	), record.m_header);
	record.m_data.m_begin = data + idx;
	record.m_data.m_len = data_len;
	idx += data_len;

	return true;
}


bool mk::bag::parse(void const* const& void_data, std::uint64_t const& len, mk::bag::records_t* const& out_records)
{
	assert(out_records);
	records_t& records = *out_records;
	records.clear();
	unsigned char const* const data = static_cast<unsigned char const*>(void_data);

	std::uint64_t idx = 0;

	static constexpr char const s_bag_magic[] = "#ROSBAG V2.0\x0A";
	static constexpr int const s_bag_magic_len = static_cast<int>(std::size(s_bag_magic)) - 1;
	CHECK_RET(len - idx >= s_bag_magic_len, false);
	CHECK_RET(std::memcmp(data + idx, s_bag_magic, s_bag_magic_len) == 0, false);
	idx += s_bag_magic_len;

	while(idx != len)
	{
		records.emplace_back();
		record_t& record = records.back();
		bool const record_parsed = detail::parse_record(data, len, idx, &record);
		CHECK_RET(record_parsed, false);
	}

	return true;
}
