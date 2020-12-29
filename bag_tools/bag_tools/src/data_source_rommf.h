#pragma once


#include "cross_platform.h"

#include <cstddef>
#include <cstdint>


namespace mk
{


	class data_source_rommf_t
	{
	public:
		data_source_rommf_t() noexcept;
		static data_source_rommf_t make(native_char_t const* const file_path);
		data_source_rommf_t(data_source_rommf_t const&) = delete;
		data_source_rommf_t(data_source_rommf_t&& other) noexcept;
		data_source_rommf_t& operator=(data_source_rommf_t const&) = delete;
		data_source_rommf_t& operator=(data_source_rommf_t&& other) noexcept;
		~data_source_rommf_t() noexcept;
		void swap(data_source_rommf_t& other) noexcept;
		explicit operator bool() const;
		void reset();
	public:
		std::uint64_t get_input_size() const;
		std::uint64_t get_input_position() const;
		std::uint64_t get_input_remaining_size() const;
		void const* get_view() const;
		std::size_t get_view_remaining_size() const;
	public:
		void consume(std::size_t const amount);
		void move_to(std::uint64_t const position, std::size_t const window_size = 2 * 1024 * 1024);
	private:
		void* m_file;
		std::uint64_t m_size;
		void* m_mapping;
		void const* m_view;
		std::uint64_t m_view_start;
		std::uint64_t m_position;
	};

	inline void swap(data_source_rommf_t& a, data_source_rommf_t& b) noexcept { a.swap(b); }


}
