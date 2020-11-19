#include "read_only_file.h"
#include "read_only_file_mapping.h"
#include "scope_exit.h"
#include "utils.h"

#include <algorithm> // std::find, std::all_of, std::any_of
#include <cassert>
#include <cstdint> // std::uint64_t, std::uint32_t
#include <cstdio> // std::puts
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <cstring> // std::memcmp, std::memcpy
#include <iterator> // std::size, std::cbegin, std::cend
#include <utility> // std::move
#include <vector>


bool bag_to_pcap(int const argc, char const* const* const argv);
bool bag_to_pcap(char const* const& input_file_name, char const* const& output_file_name);


int main(int const argc, char const* const* const argv)
{
	auto something_wrong = mk::make_scope_exit([](){ std::puts("Oh no! Something went wrong!"); });

	bool const bussiness = bag_to_pcap(argc, argv);
	CHECK_RET(bussiness, EXIT_FAILURE);

	something_wrong.reset();
	std::puts("We didn't crash. Great success!");
	return EXIT_SUCCESS;
}


bool bag_to_pcap(int const argc, char const* const* const argv)
{
	CHECK_RET(argc == 3, false);
	bool const bussiness = bag_to_pcap(argv[1], argv[2]);
	CHECK_RET(bussiness, false);
	return true;
}

bool bag_to_pcap(char const* const& input_file_name, char const* const& output_file_name)
{
	static constexpr int const s_min_file_size = 1 * 1024 * 1024;
	static constexpr char const s_bag_magic[] = "#ROSBAG V2.0";
	static constexpr int const s_bag_magic_len = static_cast<int>(std::size(s_bag_magic)) - 1;

	struct string_t
	{
		char const* m_begin;
		int m_len;
	};

	struct data_t
	{
		unsigned char const* m_begin;
		int m_len;
	};

	struct field_t
	{
		string_t m_name;
		data_t m_value;
	};

	struct header_t
	{
		std::vector<field_t> m_fields;
	};

	struct record_t
	{
		header_t m_header;
		data_t m_data;
	};

	static constexpr auto const s_fn_skip_line_ends = [](unsigned char const* const& input_file_ptr, std::uint64_t const& input_file_size, std::uint64_t& input_file_idx) -> bool
	{
		while(input_file_ptr[input_file_idx] == '\x0A')
		{
			++input_file_idx;
			if(input_file_idx == input_file_size)
			{
				return false;
			}
		}
		return true;
	};

	static constexpr auto const s_fn_read = []<typename t>(unsigned char const* const& input_file_ptr, std::uint64_t const& input_file_size, std::uint64_t& input_file_idx) -> t
	{
		CHECK_RET_CRASH(input_file_idx + sizeof(t) <= input_file_size);
		t val;
		std::memcpy(&val, input_file_ptr + input_file_idx, sizeof(t));
		input_file_idx += sizeof(t);
		return val;
	};

	static constexpr auto const s_fn_read_field = [](unsigned char const* const& input_file_ptr, std::uint64_t const& input_file_size, std::uint64_t& input_file_idx, std::uint32_t const& header_len, std::uint32_t& header_idx, field_t* const& out_field) -> bool
	{
		static constexpr int const s_max_field_len = 64 * 1024;
		static constexpr unsigned char s_field_name_value_separator = static_cast<unsigned char>('=');

		assert(out_field);
		field_t& field = *out_field;

		CHECK_RET(header_idx + sizeof(std::uint32_t) <= header_len, false);
		std::uint32_t const field_len = s_fn_read.operator()<std::uint32_t>(input_file_ptr, input_file_size, input_file_idx);
		header_idx += sizeof(std::uint32_t);
		CHECK_RET(field_len <= s_max_field_len, false);
		CHECK_RET(header_idx + field_len <= header_len, false);

		auto const it = std::find(input_file_ptr + input_file_idx, input_file_ptr + input_file_idx + field_len, s_field_name_value_separator);
		CHECK_RET(it != input_file_ptr + input_file_idx + field_len, false);
		CHECK_RET(std::all_of(input_file_ptr + input_file_idx, it, [](unsigned char const& e){ return e >= 0x20 && e <= 0x7F; }), false);

		field.m_name.m_begin = reinterpret_cast<char const*>(input_file_ptr) + input_file_idx;
		field.m_name.m_len = static_cast<int>(reinterpret_cast<char const*>(it) - field.m_name.m_begin);
		field.m_value.m_begin = it + 1;
		field.m_value.m_len = static_cast<int>(input_file_ptr + input_file_idx + field_len - field.m_value.m_begin);
		input_file_idx += field_len;
		header_idx += field_len;

		return true;
	};

	static constexpr auto const s_fn_read_header = [](unsigned char const* const& input_file_ptr, std::uint64_t const& input_file_size, std::uint64_t& input_file_idx, std::uint32_t const& header_len, header_t* const& out_header) -> bool
	{
		static constexpr char const s_op_field_name[] = "op";
		static constexpr int const s_op_field_name_len = static_cast<int>(std::size(s_op_field_name)) - 1;
		static constexpr unsigned char const s_op_value_message_data = 0x02;
		static constexpr unsigned char const s_op_value_bag_header = 0x03;
		static constexpr unsigned char const s_op_value_index_data = 0x04;
		static constexpr unsigned char const s_op_value_chunk = 0x05;
		static constexpr unsigned char const s_op_value_chunk_info = 0x06;
		static constexpr unsigned char const s_op_value_connection = 0x07;
		static constexpr unsigned char const s_op_values[] = {s_op_value_message_data, s_op_value_bag_header, s_op_value_index_data, s_op_value_chunk, s_op_value_chunk_info, s_op_value_connection};

		using std::cbegin;
		using std::cend;

		assert(out_header);
		header_t& header = *out_header;
		header.m_fields.clear();

		std::uint32_t header_idx = 0;
		while(header_idx != header_len)
		{
			field_t field;
			bool const field_read = s_fn_read_field(input_file_ptr, input_file_size, input_file_idx, header_len, header_idx, &field);
			CHECK_RET(field_read, false);
			header.m_fields.emplace_back(std::move(field));
		}
		CHECK_RET(std::any_of(header.m_fields.cbegin(), header.m_fields.cend(), [](field_t const& f)
		{
			return
				(f.m_name.m_len == s_op_field_name_len && std::memcmp(f.m_name.m_begin, s_op_field_name, s_op_field_name_len) == 0 && f.m_value.m_len == 1) &&
				(std::any_of(cbegin(s_op_values), cend(s_op_values), [&](unsigned char const& e){ return e == f.m_value.m_begin[0]; }));
		}), false);

		return true;
	};

	static constexpr auto const s_fn_read_record = [](unsigned char const* const& input_file_ptr, std::uint64_t const& input_file_size, std::uint64_t& input_file_idx, record_t* const& out_record) -> bool
	{
		static constexpr int const s_max_header_len = 1 * 1024 * 1024;
		static constexpr std::uint64_t const s_max_data_len = 2ull * 1024ull * 1024ull * 1024ull;

		assert(out_record);
		record_t& record = *out_record;

		CHECK_RET(input_file_idx + sizeof(std::uint32_t) <= input_file_size, false);
		std::uint32_t const header_len = s_fn_read.operator()<std::uint32_t>(input_file_ptr, input_file_size, input_file_idx);
		CHECK_RET(header_len <= s_max_header_len, false);
		CHECK_RET(input_file_idx + header_len <= input_file_size, false);
		bool const header_read = s_fn_read_header(input_file_ptr, input_file_size, input_file_idx, header_len, &record.m_header);
		CHECK_RET(header_read, false);

		CHECK_RET(input_file_idx + sizeof(std::uint32_t) <= input_file_size, false);
		std::uint32_t const data_len = s_fn_read.operator()<std::uint32_t>(input_file_ptr, input_file_size, input_file_idx);
		CHECK_RET(data_len <= s_max_data_len, false);
		CHECK_RET(input_file_idx + data_len <= input_file_size, false);
		record.m_data.m_begin = input_file_ptr + input_file_idx;
		record.m_data.m_len = input_file_ptr + input_file_idx + data_len - record.m_data.m_begin;
		input_file_idx += data_len;

		return true;
	};

	mk::read_only_file_t const input_file{input_file_name};
	CHECK_RET(input_file, false);
	mk::read_only_file_mapping_t const input_file_mapping{input_file};
	CHECK_RET(input_file_mapping, false);

	unsigned char const* const input_file_ptr = static_cast<unsigned char const*>(input_file_mapping.get());
	std::uint64_t const input_file_size = input_file_mapping.get_size();
	std::uint64_t input_file_idx = 0;

	CHECK_RET(input_file_size >= s_min_file_size, false);
	CHECK_RET(std::memcmp(input_file_ptr + input_file_idx, s_bag_magic, s_bag_magic_len) == 0, false);
	input_file_idx += s_bag_magic_len;
	bool const skipped = s_fn_skip_line_ends(input_file_ptr, input_file_size, input_file_idx);
	CHECK_RET(skipped, false);

	std::vector<record_t> records;
	while(input_file_idx != input_file_size)
	{
		record_t record;
		bool const record_read = s_fn_read_record(input_file_ptr, input_file_size, input_file_idx, &record);
		CHECK_RET(record_read, false);
		records.emplace_back(std::move(record));
	}

	(void)output_file_name;

	return true;
}
