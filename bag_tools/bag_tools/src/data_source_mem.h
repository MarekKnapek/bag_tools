#pragma once


#include <cstddef>
#include <cstdint>


namespace mk
{


	class data_source_mem_t
	{
	public:
		data_source_mem_t() noexcept;
		static data_source_mem_t make(void const* const data, std::size_t const size);
		data_source_mem_t(data_source_mem_t const&) = delete;
		data_source_mem_t(data_source_mem_t&& other) noexcept;
		data_source_mem_t& operator=(data_source_mem_t const&) = delete;
		data_source_mem_t& operator=(data_source_mem_t&& other) noexcept;
		~data_source_mem_t() noexcept;
		void swap(data_source_mem_t& other) noexcept;
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
		void const* m_data;
		std::size_t m_size;
		std::size_t m_position;
	};

	inline void swap(data_source_mem_t& a, data_source_mem_t& b) noexcept { a.swap(b); }


}
