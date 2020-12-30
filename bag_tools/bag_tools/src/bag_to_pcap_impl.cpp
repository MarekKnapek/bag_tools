#include "bag_to_pcap_impl.h"

#include "data_source_mem.h"
#include "data_source_rommf.h"
#include "overload.h"
#include "read_only_memory_mapped_file.h"
#include "scope_exit.h"
#include "utils.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <optional>

#include <lz4frame.h>


static std::chrono::milliseconds g_time;
static std::ofstream g_ofs;


bool mk::bag_tool::detail::bag_to_pcap(int const argc, native_char_t const* const* const argv)
{
	CHECK_RET_F(argc == 4);
	return bag_to_pcap(argv[2], argv[3]);
}


bool mk::bag_tool::detail::bag_to_pcap(native_char_t const* const input_bag, native_char_t const* const output_pcap)
{
	mk::read_only_memory_mapped_file_t const rommf{input_bag};
	if(rommf)
	{
		mk::data_source_mem_t data_source_mem = mk::data_source_mem_t::make(rommf.get_data(), static_cast<std::size_t>(rommf.get_size()));
		CHECK_RET_F(data_source_mem);
		bool const converted = bag_to_pcap(data_source_mem, output_pcap);
		CHECK_RET_F(converted);
		return true;
	}
	else
	{
		mk::data_source_rommf_t data_source_rommf = mk::data_source_rommf_t::make(input_bag);
		CHECK_RET_F(data_source_rommf);
		bool const converted = bag_to_pcap(data_source_rommf, output_pcap);
		CHECK_RET_F(converted);
		return true;
	}
}

template<typename data_source_t>
bool mk::bag_tool::detail::bag_to_pcap(data_source_t& data_source, native_char_t const* const output_pcap)
{
	CHECK_RET_F(mk::bag2::is_bag_file(data_source));
	data_source.consume(mk::bag2::bag_file_header_len());

	std::uint32_t ouster_channel;
	bool const got_ouster_channel = get_ouster_channel(data_source, &ouster_channel);
	CHECK_RET_F(got_ouster_channel);

	g_time = std::chrono::milliseconds{0};
	g_ofs = std::ofstream{output_pcap, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc};

	struct pcap_hdr_t
	{
		std::uint32_t magic_number; /* magic number */
		std::uint16_t version_major; /* major version number */
		std::uint16_t version_minor; /* minor version number */
		std::int32_t thiszone; /* GMT to local correction */
		std::uint32_t sigfigs; /* accuracy of timestamps */
		std::uint32_t snaplen; /* max length of captured packets, in octets */
		std::uint32_t network; /* data link type */
	};
	static_assert(sizeof(pcap_hdr_t) == 24);
	pcap_hdr_t pcap_hdr;
	pcap_hdr.magic_number = 0xa1b2c3d4;
	pcap_hdr.version_major = 2;
	pcap_hdr.version_minor = 4;
	pcap_hdr.thiszone = 0;
	pcap_hdr.sigfigs = 0;
	pcap_hdr.snaplen = 64 * 1024;
	pcap_hdr.network = 1;
	g_ofs.write(reinterpret_cast<char const*>(&pcap_hdr), sizeof(pcap_hdr));

	data_source.move_to(0, mk::bag2::bag_file_header_len());
	data_source.consume(mk::bag2::bag_file_header_len());
	bool const ouster_records_processed = process_ouster_records(data_source, ouster_channel);
	CHECK_RET_F(ouster_records_processed);

	g_ofs.close();

	return true;
}

template<typename data_source_t>
bool mk::bag_tool::detail::get_ouster_channel(data_source_t& data_source, std::uint32_t* const out_ouster_channel)
{
	assert(out_ouster_channel);
	std::uint32_t& ouster_channel = *out_ouster_channel;

	static constexpr auto const s_record_callback = [](void* const ctx, mk::bag2::callback_variant_e const variant, void* const data) -> bool
	{
		std::optional<std::uint32_t>& ouster_channel_opt = *static_cast<std::optional<std::uint32_t>*>(ctx);
		CHECK_RET_F(variant == mk::bag2::callback_variant_e::record);
		mk::bag2::record_t const& record = *static_cast<mk::bag2::record_t const*>(data);

		bool const processed = get_ouster_channel_record(record, &ouster_channel_opt);
		CHECK_RET_F(processed);

		return true;
	};
	mk::bag2::callback_t const callback = s_record_callback;

	std::optional<std::uint32_t> ouster_channel_opt = std::nullopt;
	bool const file_parsed = mk::bag2::parse_records(data_source, callback, &ouster_channel_opt);
	CHECK_RET_F(file_parsed);

	CHECK_RET_F(ouster_channel_opt.has_value());
	ouster_channel = *ouster_channel_opt;

	return true;
}

bool mk::bag_tool::detail::get_ouster_channel_record(mk::bag2::record_t const& record, std::optional<std::uint32_t>* const out_ouster_channel_opt)
{
	assert(out_ouster_channel_opt);
	std::optional<std::uint32_t>& ouster_channel_opt = *out_ouster_channel_opt;

	bool is_my_topic;
	bool const filtered = is_topic_ouster_lidar_packets(record, &is_my_topic);
	CHECK_RET_F(filtered);
	if(!is_my_topic)
	{
		return true;
	}

	CHECK_RET_F(!ouster_channel_opt.has_value());
	ouster_channel_opt = std::get<mk::bag2::header::connection_t>(record.m_header).m_conn;

	return true;
}

bool mk::bag_tool::detail::is_topic_ouster_lidar_packets(mk::bag2::record_t const& record, bool* const out_satisfies)
{
	static constexpr char const s_topic_ouster_0_lidar_packets_name[] = "/os_node/lidar_packets";
	static constexpr int const s_topic_ouster_0_lidar_packets_name_len = static_cast<int>(std::size(s_topic_ouster_0_lidar_packets_name)) - 1;
	static constexpr char const s_topic_ouster_1a_lidar_packets_name[] = "/os1_node/lidar_packets";
	static constexpr int const s_topic_ouster_1a_lidar_packets_name_len = static_cast<int>(std::size(s_topic_ouster_1a_lidar_packets_name)) - 1;
	static constexpr char const s_topic_ouster_1b_lidar_packets_name[] = "os1_node/lidar_packets";
	static constexpr int const s_topic_ouster_1b_lidar_packets_name_len = static_cast<int>(std::size(s_topic_ouster_1b_lidar_packets_name)) - 1;

	assert(out_satisfies);
	bool& satisfies = *out_satisfies;

	bool const is_connection = std::visit(mk::make_overload([](mk::bag2::header::connection_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
	if(!is_connection)
	{
		satisfies = false;
		return true;
	}
	mk::bag2::header::connection_t const& connection = std::get<mk::bag2::header::connection_t>(record.m_header);

	bool const is_ouster_0 = connection.m_topic.m_len == s_topic_ouster_0_lidar_packets_name_len && std::memcmp(connection.m_topic.m_begin, s_topic_ouster_0_lidar_packets_name, s_topic_ouster_0_lidar_packets_name_len) == 0;
	if(is_ouster_0)
	{
		satisfies = true;
		return true;
	}
	bool const is_ouster_1a = connection.m_topic.m_len == s_topic_ouster_1a_lidar_packets_name_len && std::memcmp(connection.m_topic.m_begin, s_topic_ouster_1a_lidar_packets_name, s_topic_ouster_1a_lidar_packets_name_len) == 0;
	if(is_ouster_1a)
	{
		satisfies = true;
		return true;
	}
	bool const is_ouster_1b = connection.m_topic.m_len == s_topic_ouster_1b_lidar_packets_name_len && std::memcmp(connection.m_topic.m_begin, s_topic_ouster_1b_lidar_packets_name, s_topic_ouster_1b_lidar_packets_name_len) == 0;
	if(is_ouster_1b)
	{
		satisfies = true;
		return true;
	}

	satisfies = false;
	return true;
}

template<typename data_source_t>
bool mk::bag_tool::detail::process_ouster_records(data_source_t& data_source, std::uint32_t const ouster_channel)
{
	struct helper_struct_t
	{
		std::uint32_t m_ouster_channel;
		std::vector<unsigned char> m_helper_buffer;
	};

	static constexpr auto const s_record_callback = [](void* const ctx, mk::bag2::callback_variant_e const variant, void* const data) -> bool
	{
		helper_struct_t& helper = *static_cast<helper_struct_t*>(ctx);
		CHECK_RET_F(variant == mk::bag2::callback_variant_e::record);
		mk::bag2::record_t const& record = *static_cast<mk::bag2::record_t const*>(data);

		bool const processed = process_record_ouster_chunk(record, helper.m_ouster_channel, helper.m_helper_buffer);
		CHECK_RET_F(processed);

		return true;
	};
	mk::bag2::callback_t const callback = s_record_callback;

	helper_struct_t helper;
	helper.m_ouster_channel = ouster_channel;
	bool const parsed = mk::bag2::parse_records(data_source, callback, &helper);
	CHECK_RET_F(parsed);

	return true;
}

bool mk::bag_tool::detail::process_record_ouster_chunk(mk::bag2::record_t const& record, std::uint32_t const ouster_channel, std::vector<unsigned char>& helper_buffer)
{
	bool const is_chunk = std::visit(mk::make_overload([](mk::bag2::header::chunk_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
	if(!is_chunk)
	{
		return true;
	}
	mk::bag2::header::chunk_t const& chunk = std::get<mk::bag2::header::chunk_t>(record.m_header);

	void const* decompressed_data;
	bool const decompressed = decompress_record_chunk_data(record, helper_buffer, &decompressed_data);
	CHECK_RET_F(decompressed);

	static constexpr auto const s_record_callback = [](void* const ctx, mk::bag2::callback_variant_e const variant, void* const data) -> bool
	{
		std::uint32_t const& ouster_channel = *static_cast<std::uint32_t const*>(ctx);
		CHECK_RET_F(variant == mk::bag2::callback_variant_e::record);
		mk::bag2::record_t const& record = *static_cast<mk::bag2::record_t const*>(data);

		bool const processed = process_inner_ouster_record(record, ouster_channel);
		CHECK_RET_F(processed);

		return true;
	};
	mk::bag2::callback_t const callback = s_record_callback;

	std::uint32_t ouster_channel_ = ouster_channel;
	mk::data_source_mem_t data_source = mk::data_source_mem_t::make(decompressed_data, chunk.m_size);
	bool const parsed = mk::bag2::parse_records(data_source, callback, &ouster_channel_);
	CHECK_RET_F(parsed);

	return true;
}

bool mk::bag_tool::detail::decompress_record_chunk_data(mk::bag2::record_t const& record, std::vector<unsigned char>& helper_buffer, void const** out_decompressed_data)
{
	static constexpr char const s_compression_none_name[] = "none";
	static constexpr int const s_compression_none_name_len = static_cast<int>(std::size(s_compression_none_name)) - 1;
	static constexpr char const s_compression_lz4_name[] = "lz4";
	static constexpr int const s_compression_lz4_name_len = static_cast<int>(std::size(s_compression_lz4_name)) - 1;

	assert(out_decompressed_data);
	assert(std::visit(mk::make_overload([](mk::bag2::header::chunk_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header));

	mk::bag2::header::chunk_t const& chunk = std::get<mk::bag2::header::chunk_t>(record.m_header);

	bool const is_none = chunk.m_compression.m_len == s_compression_none_name_len && std::memcmp(chunk.m_compression.m_begin, s_compression_none_name, s_compression_none_name_len) == 0;
	if(is_none)
	{
		void const*& decompressed_data = *out_decompressed_data;
		decompressed_data = record.m_data.m_begin;
		return true;
	}

	bool const is_lz4 = chunk.m_compression.m_len == s_compression_lz4_name_len && std::memcmp(chunk.m_compression.m_begin, s_compression_lz4_name, s_compression_lz4_name_len) == 0;
	if(is_lz4)
	{
		helper_buffer.resize(chunk.m_size);
		bool const decompressed = decompress_lz4(record.m_data.m_begin, record.m_data.m_len, helper_buffer.data(), static_cast<int>(chunk.m_size));
		CHECK_RET(decompressed, false);

		void const*& decompressed_data = *out_decompressed_data;
		decompressed_data = helper_buffer.data();
		return true;
	}

	return false;
}

bool mk::bag_tool::detail::decompress_lz4(void const* const input, int const input_len_, void* const output, int const output_len_)
{
	LZ4F_decompressionContext_t ctx;
	LZ4F_errorCode_t const context_created = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
	CHECK_RET_F(!LZ4F_isError(context_created));
	auto const context_free = mk::make_scope_exit([&](){ LZ4F_errorCode_t const context_freed = LZ4F_freeDecompressionContext(ctx); CHECK_RET_CRASH(context_freed == 0); });

	std::size_t output_len = output_len_;
	std::size_t input_len = input_len_;
	std::size_t const decompressed = LZ4F_decompress(ctx, output, &output_len, input, &input_len, nullptr);
	CHECK_RET_F(decompressed == 0 && output_len == static_cast<std::size_t>(output_len_) && input_len == static_cast<std::size_t>(input_len_));

	return true;
}

bool mk::bag_tool::detail::process_inner_ouster_record(mk::bag2::record_t const& record, std::uint32_t const ouster_channel)
{
	static constexpr int const s_udp_header_len = 8;
	static constexpr int const s_ip_header_len = 28;
	static constexpr int const s_brutal_header_len = 42;
	static constexpr int const s_payload_len = 12608;
	static constexpr int const s_pcap_payload_len = s_payload_len + s_brutal_header_len; // 12650
	static constexpr int const s_bag_payload_len = 12613;
	static constexpr std::uint16_t const s_destination_port_number = 7502;

	bool const is_message_data = std::visit(mk::make_overload([](mk::bag2::header::message_data_t const&) -> bool { return true; }, [](...) -> bool { return false; }), record.m_header);
	if(!is_message_data)
	{
		return true;
	}
	mk::bag2::header::message_data_t const& message_data = std::get<mk::bag2::header::message_data_t>(record.m_header);
	bool const is_my_connection = message_data.m_conn == ouster_channel;
	if(!is_my_connection)
	{
		return true;
	}
	bool const is_good_size = record.m_data.m_len == s_bag_payload_len;
	if(!is_good_size)
	{
		return true;
	}

	struct pcaprec_hdr_t
	{
		std::uint32_t ts_sec; /* timestamp seconds */
		std::uint32_t ts_usec; /* timestamp microseconds */
		std::uint32_t incl_len; /* number of octets of packet saved in file */
		std::uint32_t orig_len; /* actual length of packet */
	};
	static_assert(sizeof(pcaprec_hdr_t) == 16);
	struct brutal_header_t
	{
		unsigned char eth_ig_1[3];
		unsigned char eth_addr_1[3];
		unsigned char eth_ig_2[3];
		unsigned char eth_addr_2[3];
		unsigned char eth_type[2];
		unsigned char ip_hdr_len[1];
		unsigned char ip_dsfield_enc[1];
		unsigned char ip_len[2];
		unsigned char ip_id[2];
		unsigned char ip_frag_offset[2];
		unsigned char ip_ttl[1];
		unsigned char ip_proto[1];
		unsigned char ip_checksum[2];
		unsigned char ip_src[4];
		unsigned char ip_dst[4];
		unsigned char udp_src_port[2];
		unsigned char udp_dst_port[2];
		unsigned char udp_len[2];
		unsigned char udp_checksum[2];
	};
	static_assert(sizeof(brutal_header_t) == 42);

	g_time += std::chrono::milliseconds{2};

	pcaprec_hdr_t pcap_record_header;
	pcap_record_header.ts_sec = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(g_time).count());
	pcap_record_header.ts_usec = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(g_time - std::chrono::duration_cast<std::chrono::seconds>(g_time)).count());
	pcap_record_header.incl_len = s_pcap_payload_len;
	pcap_record_header.orig_len = s_pcap_payload_len;
	g_ofs.write(reinterpret_cast<char const*>(&pcap_record_header), sizeof(pcap_record_header));

	brutal_header_t brutal_header{};
	brutal_header.eth_ig_1[0] = 0xFF;
	brutal_header.eth_ig_1[1] = 0xFF;
	brutal_header.eth_ig_1[2] = 0xFF;
	brutal_header.eth_addr_1[0] = 0xFF;
	brutal_header.eth_addr_1[1] = 0xFF;
	brutal_header.eth_addr_1[2] = 0xFF;
	brutal_header.eth_ig_2[0] = 0x60;
	brutal_header.eth_ig_2[1] = 0x76;
	brutal_header.eth_ig_2[2] = 0x88;
	brutal_header.eth_addr_2[0] = 0x00;
	brutal_header.eth_addr_2[1] = 0x00;
	brutal_header.eth_addr_2[2] = 0x00;
	brutal_header.eth_type[0] = 0x08;
	brutal_header.eth_type[1] = 0x00;
	brutal_header.ip_hdr_len[0] = 0x45;
	brutal_header.ip_dsfield_enc[0] = 0x00;
	brutal_header.ip_len[0] = ((s_ip_header_len + s_payload_len) >> (1 * 8)) & 0xFF;
	brutal_header.ip_len[1] = ((s_ip_header_len + s_payload_len) >> (0 * 8)) & 0xFF;
	brutal_header.ip_id[0] = 0x00;
	brutal_header.ip_id[1] = 0x00;
	brutal_header.ip_frag_offset[0] = 0x40;
	brutal_header.ip_frag_offset[1] = 0x00;
	brutal_header.ip_ttl[0] = 0xFF;
	brutal_header.ip_proto[0] = 0x11;
	brutal_header.ip_checksum[0] = 0x00; // ???
	brutal_header.ip_checksum[1] = 0x00; // ???
	brutal_header.ip_src[0] = (192) & 0xFF;
	brutal_header.ip_src[1] = (168) & 0xFF;
	brutal_header.ip_src[2] = (2) & 0xFF;
	brutal_header.ip_src[3] = (2) & 0xFF;
	brutal_header.ip_dst[0] = 0xFF;
	brutal_header.ip_dst[1] = 0xFF;
	brutal_header.ip_dst[2] = 0xFF;
	brutal_header.ip_dst[3] = 0xFF;
	brutal_header.udp_src_port[0] = ((s_destination_port_number + 0) >> (1 * 8)) & 0xFF;
	brutal_header.udp_src_port[1] = ((s_destination_port_number + 0) >> (0 * 8)) & 0xFF;
	brutal_header.udp_dst_port[0] = ((s_destination_port_number + 0) >> (1 * 8)) & 0xFF;
	brutal_header.udp_dst_port[1] = ((s_destination_port_number + 0) >> (0 * 8)) & 0xFF;
	brutal_header.udp_len[0] = ((s_udp_header_len + s_payload_len) >> (1 * 8)) & 0xFF;
	brutal_header.udp_len[1] = ((s_udp_header_len + s_payload_len) >> (0 * 8)) & 0xFF;
	brutal_header.udp_checksum[0] = 0x00;
	brutal_header.udp_checksum[1] = 0x00;
	g_ofs.write((char const*)&brutal_header, sizeof(brutal_header));

	g_ofs.write(reinterpret_cast<char const*>(record.m_data.m_begin + 4), record.m_data.m_len - 5);

	return true;
}
