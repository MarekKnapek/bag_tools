#pragma once


#include "bag.h"
#include "cross_platform.h"


namespace mk
{
	namespace bag_tool
	{
		namespace detail
		{


			struct bag_info_t
			{
			public:
				bag_info_t();
			public:
				unsigned m_counter;
				mk::bag::header::bag_t m_bag_hdr;
			};


			bool bag_info(int const argc, native_char_t const* const* const argv);

			bool bag_info(native_char_t const* const input_bag);
			template<typename data_source_t> bool bag_info(data_source_t& data_source);
			bool process_record(bag_info_t& bag_info, mk::bag::record_t const& record);
			char const* get_record_type_name(mk::bag::record_t const& record);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::bag_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::chunk_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::connection_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::message_data_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::index_data_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag::record_t const& record, mk::bag::header::chunk_info_t const& /* tag */);


		}
	}
}
