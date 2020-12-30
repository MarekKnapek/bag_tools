#pragma once


#include "bag.h"
#include "cross_platform.h"

#include <cstdint>
#include <vector>


namespace mk
{
	namespace bag_tool
	{
		namespace detail
		{


			bool bag_to_pcap(int const argc, native_char_t const* const* const argv);

			bool bag_to_pcap(native_char_t const* const input_bag, native_char_t const* const output_pcap);
			template<typename data_source_t>
			bool bag_to_pcap(data_source_t& data_source, native_char_t const* const output_pcap);
			template<typename data_source_t>
			bool get_ouster_channel(data_source_t& data_source, std::uint32_t* const out_ouster_channel);
			bool get_ouster_channel_record(mk::bag2::record_t const& record, std::optional<std::uint32_t>* const out_ouster_channel_opt);
			bool is_topic_ouster_lidar_packets(mk::bag2::record_t const& record, bool* const out_satisfies);
			template<typename data_source_t>
			bool process_ouster_records(data_source_t& data_source, std::uint32_t const ouster_channel);
			bool process_record_ouster_chunk(mk::bag2::record_t const& record, std::uint32_t const ouster_channel, std::vector<unsigned char>& helper_buffer);
			bool decompress_record_chunk_data(mk::bag2::record_t const& record, std::vector<unsigned char>& helper_buffer, void const** out_decompressed_data);
			bool decompress_lz4(void const* const input, int const input_len, void* const output, int const output_len);
			bool process_inner_ouster_record(mk::bag2::record_t const& record, std::uint32_t const ouster_channel);


		}
	}
}
