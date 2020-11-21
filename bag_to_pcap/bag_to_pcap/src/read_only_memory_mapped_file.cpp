#include "read_only_memory_mapped_file.h"

#include "utils.h"

#include <utility> // std::swap
#include <cassert>


mk::read_only_memory_mapped_file_t::read_only_memory_mapped_file_t() noexcept :
	m_native_file()
{
}

mk::read_only_memory_mapped_file_t::read_only_memory_mapped_file_t(native_char_t const* const& file_path) :
	m_native_file(file_path)
{
}

mk::read_only_memory_mapped_file_t::read_only_memory_mapped_file_t(read_only_memory_mapped_file_t&& other) noexcept :
	read_only_memory_mapped_file_t()
{
	swap(other);
}

mk::read_only_memory_mapped_file_t& mk::read_only_memory_mapped_file_t::operator=(read_only_memory_mapped_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_memory_mapped_file_t::~read_only_memory_mapped_file_t() noexcept
{
}

void mk::read_only_memory_mapped_file_t::swap(read_only_memory_mapped_file_t& other) noexcept
{
	using std::swap;
	swap(m_native_file, other.m_native_file);
}

mk::read_only_memory_mapped_file_t::operator bool() const
{
	return m_native_file.operator bool();
}

void const* mk::read_only_memory_mapped_file_t::get_data() const
{
	return m_native_file.get_data();
}

std::uint64_t mk::read_only_memory_mapped_file_t::get_size() const
{
	return m_native_file.get_size();
}
