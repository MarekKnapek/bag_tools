#pragma once


#include <cstdint> // std::uint64_t, std::uint32_t
#include <variant>
#include <vector>


namespace mk
{
	namespace bag
	{


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


		namespace header
		{

			struct bag_t
			{
				static constexpr int const s_op = 3;
				std::uint64_t m_index_pos; // offset of first record after the chunk section
				std::uint32_t m_conn_count; // number of unique connections in the file
				std::uint32_t m_chunk_count; // number of chunk records in the file
			};

			struct chunk_t
			{
				static constexpr int const s_op = 5;
				string_t m_compression; // compression type for the data
				std::uint32_t m_size; // size in bytes of the uncompressed chunk
			};

			struct connection_t
			{
				static constexpr int const s_op = 7;
				std::uint32_t m_conn; // unique connection ID
				string_t m_topic; // topic on which the messages are stored
			};

			struct message_data_t
			{
				static constexpr int const s_op = 2;
				std::uint32_t m_conn; // ID for connection on which message arrived
				std::uint64_t m_time; // time at which the message was received
			};

			struct index_data_t
			{
				static constexpr int const s_op = 4;
				std::uint32_t m_ver; // index data record version
				std::uint32_t m_conn; // connection ID
				std::uint32_t m_count; // number of messages on conn in the preceding chunk
			};

			struct chunk_info_t
			{
				static constexpr int const s_op = 6;
				std::uint32_t m_ver; // chunk info record version
				std::uint64_t m_chunk_pos; // offset of the chunk record
				std::uint64_t m_start_time; // timestamp of earliest message in the chunk
				std::uint64_t m_end_time; // timestamp of latest message in the chunk
				std::uint32_t m_count; // number of connections in the chunk
			};

			typedef std::variant<bag_t, chunk_t, connection_t, message_data_t, index_data_t, chunk_info_t> header_t;

		}

		namespace data
		{

			struct index_data_ver_1_t
			{
				std::uint64_t m_time; // time at which the message was received
				std::uint32_t m_offset; // offset of message data record in uncompressed chunk data
			};

			struct chunk_info_ver_1_t
			{
				std::uint32_t m_conn; // connection id
				std::uint32_t m_count; // number of messages that arrived on this connection in the chunk
			};

		}


		struct field_t
		{
			string_t m_name;
			data_t m_value;
		};

		struct raw_header_t
		{
			std::vector<field_t> m_fields;
		};

		struct raw_record_t
		{
			raw_header_t m_header;
			data_t m_data;
		};
		typedef std::vector<raw_record_t> raw_records_t;

		struct record_t
		{
			header::header_t m_header;
			data_t m_data;
		};
		typedef std::vector<record_t> records_t;


		bool parse(void const* const& data, std::uint64_t const& len, records_t* const& out_records);


	}
}
