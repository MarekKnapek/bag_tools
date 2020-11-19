#include "read_only_file_mapping.h"

#include "read_only_file.h"
#include "utils.h"

#include <cassert>
#include <utility> // std::swap

// fstat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/mman.h> // map, unmap


mk::read_only_file_mapping_t::read_only_file_mapping_t() noexcept :
	m_mapping(),
	m_size()
{
}

mk::read_only_file_mapping_t::read_only_file_mapping_t(mk::read_only_file_t const& read_only_file) :
	mk::read_only_file_mapping_t()
{
	assert(read_only_file);

	struct stat stat_buff;
	int const stated = fstat(read_only_file.get(), &stat_buff);
	CHECK_RET_V(stated == 0);

	void* const mapping = mmap(nullptr, stat_buff.st_size, PROT_READ, MAP_PRIVATE, read_only_file.get(), 0);
	CHECK_RET_V(mapping != MAP_FAILED && mapping != nullptr);
	m_mapping = mapping;
	m_size = stat_buff.st_size;
}

mk::read_only_file_mapping_t::read_only_file_mapping_t(mk::read_only_file_mapping_t&& other) noexcept :
	mk::read_only_file_mapping_t()
{
	swap(other);
}

mk::read_only_file_mapping_t& mk::read_only_file_mapping_t::operator=(mk::read_only_file_mapping_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_file_mapping_t::~read_only_file_mapping_t() noexcept
{
	if(!*this)
	{
		return;
	}
	int const munmapped = munmap(m_mapping, m_size);
	CHECK_RET_V(munmapped == 0);
}

void mk::read_only_file_mapping_t::swap(mk::read_only_file_mapping_t& other) noexcept
{
	using std::swap;
	swap(m_mapping, other.m_mapping);
}

void const* mk::read_only_file_mapping_t::get() const
{
	return m_mapping;
}

std::uint64_t const& mk::read_only_file_mapping_t::get_size() const
{
	return m_size;
}

mk::read_only_file_mapping_t::operator bool() const
{
	return m_mapping != nullptr;
}
