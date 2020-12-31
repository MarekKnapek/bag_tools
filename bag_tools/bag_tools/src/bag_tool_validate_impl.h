#pragma once


#include "bag.h"
#include "cross_platform.h"


namespace mk
{
	namespace bag_tool
	{
		namespace detail
		{


			enum class data_source_e
			{
				mem,
				rommf,
			};

			enum class connection_automat_e
			{
				before,
				during,
				after,
			};


			struct bag_validate_t
			{
			public:
				bag_validate_t();
			public:
				void* m_data_source;
				int m_data_source_type;
				unsigned m_counter;
				mk::bag::header::bag_t m_bag_hdr;
				connection_automat_e m_connection_automat;
			};


			bool bag_validate(int const argc, native_char_t const* const* const argv);

			bool bag_validate(native_char_t const* const input_bag);
			template<typename data_source_t> bool bag_validate(bag_validate_t& bag_validate, data_source_t& data_source);
			bool process_record(bag_validate_t& bag_validate, mk::bag::record_t const& record);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::bag_t const& /* tag */);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::chunk_t const& /* tag */);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::connection_t const& /* tag */);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::message_data_t const& /* tag */);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::index_data_t const& /* tag */);
			bool process_type(bag_validate_t& bag_validate, mk::bag::record_t const& record, mk::bag::header::chunk_info_t const& /* tag */);


		}
	}
}
