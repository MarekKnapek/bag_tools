#pragma once


#include "bag2.h"
#include "cross_platform.h"


namespace mk
{
	namespace bag_tools
	{
		namespace detail
		{


			struct bag_info_t
			{
			public:
				bag_info_t();
			public:
				unsigned m_counter;
				mk::bag2::header::bag_t m_bag_hdr;
			};


			bool bag_info(native_char_t const* const input_bag);

			template<typename data_source_t> bool bag_info(data_source_t& data_source);
			bool process_record(bag_info_t& bag_info, mk::bag2::record_t const& record);
			char const* get_record_type_name(mk::bag2::record_t const& record);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::bag_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::chunk_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::connection_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::message_data_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::index_data_t const& /* tag */);
			bool process_type(bag_info_t& bag_info, mk::bag2::record_t const& record, mk::bag2::header::chunk_info_t const& /* tag */);


		}
	}
}
