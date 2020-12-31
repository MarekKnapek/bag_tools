#pragma once


#include <array>
#include <cstdint> // std::uint64_t, std::uint32_t, std::uint8_t
#include <optional>
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

		struct md5sum_t
		{
			std::uint64_t m_lo;
			std::uint64_t m_hi;
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

			struct connection_data_t
			{
				std::optional<string_t> m_caller_id; // name of node sending data
				string_t m_topic; // name of the topic the subscriber is connecting to
				std::optional<string_t> m_service; // name of service the client is calling
				md5sum_t m_md5sum; // md5sum of the message type
				string_t m_type; // message type
				string_t m_message_definition; // full text of message definition (output of gendeps --cat)
				std::optional<string_t> m_error; // human-readable error message if the connection is not successful
				std::optional<std::uint8_t> /*???*/ m_persistent; // sent from a service client to a service. If '1', keep connection open for multiple requests.
				std::optional<std::uint8_t> /*???*/ m_tcp_nodelay; // sent from subscriber to publisher. If '1', publisher will set TCP_NODELAY on socket if possible
				std::optional<std::uint8_t> m_latching; // publisher is in latching mode (i.e. sends the last value published to new subscribers)
			};

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
		static constexpr int const s_fields_max = 8;
		typedef std::array<field_t, s_fields_max> fields_t;

		struct record_t
		{
			header::header_t m_header;
			data_t m_data;
		};


		typedef bool(*callback_t)(void* const ctx, void* const data);


		int bag_file_header_len();
		template<typename data_source_t>
		bool is_bag_file(data_source_t& data_source);
		void const* adjust_data(void const* const& data);
		std::uint64_t adjust_len(std::uint64_t const& len);
		template<typename data_source_t>
		bool parse_records(data_source_t& data_source, callback_t const callback, void* const callback_ctx);
		template<typename data_source_t>
		bool parse_fields(data_source_t& data_source, callback_t const callback, void* const callback_ctx);
		bool parse_connection_data(field_t const* const& fields, int const& fields_count, data::connection_data_t* const& out_connection_data);


	}
}
