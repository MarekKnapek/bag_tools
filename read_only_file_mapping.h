#pragma once


#include <cstdint> // std::uint64_t


namespace mk { class read_only_file_t; }


namespace mk
{


	class read_only_file_mapping_t
	{
	public:
		read_only_file_mapping_t() noexcept;
		read_only_file_mapping_t(mk::read_only_file_t const& read_only_file);
		read_only_file_mapping_t(mk::read_only_file_mapping_t const&) = delete;
		read_only_file_mapping_t(mk::read_only_file_mapping_t&& other) noexcept;
		mk::read_only_file_mapping_t& operator=(mk::read_only_file_mapping_t const&) = delete;
		mk::read_only_file_mapping_t& operator=(mk::read_only_file_mapping_t&& other) noexcept;
		~read_only_file_mapping_t() noexcept;
		void swap(mk::read_only_file_mapping_t& other) noexcept;
	public:
		void const* get() const;
		std::uint64_t const& get_size() const;
		explicit operator bool() const;
	private:
		void* m_mapping;
		std::uint64_t m_size;
	};

	inline void swap(mk::read_only_file_mapping_t& a, mk::read_only_file_mapping_t& b) noexcept { a.swap(b); }


}
