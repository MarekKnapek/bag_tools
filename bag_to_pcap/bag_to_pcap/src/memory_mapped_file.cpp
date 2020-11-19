#include "memory_mapped_file.h"

#include "utils.h"

#include <algorithm>
#include <cassert>
#include <utility>

#include <windows.h>


#define s_very_big_int (2'147'483'647)
static constexpr int const s_max_file_size = s_very_big_int;


void mapped_view_deleter::operator()(void const* const ptr) const
{
	BOOL const unmapped = UnmapViewOfFile(ptr);
	assert(unmapped != 0);
}


memory_mapped_file::memory_mapped_file() noexcept :
	m_file(),
	m_mapping(),
	m_view(),
	m_size()
{
}

memory_mapped_file::memory_mapped_file(wchar_t const* const file_name) :
	memory_mapped_file()
{
	HANDLE const file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	CHECK_RET_V(file != INVALID_HANDLE_VALUE);
	smart_handle s_file(file);
	LARGE_INTEGER size;
	BOOL const got_size = GetFileSizeEx(file, &size);
	assert(got_size != 0);
	CHECK_RET_V(size.HighPart == 0);
	CHECK_RET_V(size.LowPart != 0);
	CHECK_RET_V(size.LowPart <= s_max_file_size);
	HANDLE const mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CHECK_RET_V(mapping != nullptr);
	smart_handle s_mapping(mapping);
	void const* const ptr = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	CHECK_RET_V(mapping != nullptr);
	smart_mapped_view s_view(ptr);

	m_file = std::move(s_file);
	m_mapping = std::move(s_mapping);
	m_view = std::move(s_view);
	m_size = static_cast<int>(size.LowPart);
}

memory_mapped_file::memory_mapped_file(memory_mapped_file&& other) noexcept :
	memory_mapped_file()
{
	swap(other);
}

memory_mapped_file& memory_mapped_file::operator=(memory_mapped_file&& other) noexcept
{
	swap(other);
	return *this;
}

memory_mapped_file::~memory_mapped_file() noexcept
{
}

void memory_mapped_file::swap(memory_mapped_file& other) noexcept
{
	using std::swap;
	swap(m_file, other.m_file);
	swap(m_mapping, other.m_mapping);
	swap(m_view, other.m_view);
	swap(m_size, other.m_size);
}

std::byte const* memory_mapped_file::begin() const
{
	return static_cast<std::byte const*>(m_view.get());
}

std::byte const* memory_mapped_file::end() const
{
	return begin() + size();
}

int memory_mapped_file::size() const
{
	return m_size;
}
