#include "data_source_rommf.h"

#include "utils.h"

#include <cassert>
#include <utility> // std::swap

#include <windows.h>


namespace mk
{
	namespace detail
	{
		static constexpr int const s_view_size = 64 * 1024 * 1024;
		static constexpr int const s_view_granularity = 2 * 1024 * 1024;
	}
}


mk::data_source_rommf_t::data_source_rommf_t() noexcept :
	m_file(INVALID_HANDLE_VALUE),
	m_size(),
	m_mapping(),
	m_view(),
	m_view_start(0xFFFFFFFFFFFFFFFFull),
	m_position(0xFFFFFFFFFFFFFFFFull)
{
}

mk::data_source_rommf_t mk::data_source_rommf_t::make(native_char_t const* const file_path)
{
	data_source_rommf_t source;

	HANDLE const file = CreateFileW(file_path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	CHECK_RET(file != INVALID_HANDLE_VALUE, source);
	source.m_file = file;

	static_assert(sizeof(std::uint64_t) >= sizeof(LARGE_INTEGER));
	LARGE_INTEGER size;
	BOOL const got_size = GetFileSizeEx(file, &size);
	CHECK_RET(got_size != 0, source);
	source.m_size = size.QuadPart;

	HANDLE const mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	CHECK_RET(mapping != nullptr, source);
	source.m_mapping = mapping;

	return source;
}

mk::data_source_rommf_t::data_source_rommf_t(data_source_rommf_t&& other) noexcept :
	data_source_rommf_t()
{
	swap(other);
}

mk::data_source_rommf_t& mk::data_source_rommf_t::operator=(data_source_rommf_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::data_source_rommf_t::~data_source_rommf_t() noexcept
{
	if(m_view != nullptr)
	{
		BOOL const unmapped = UnmapViewOfFile(m_view);
		CHECK_RET_CRASH(unmapped != 0);
	}
	if(m_mapping != nullptr)
	{
		BOOL const closed = CloseHandle(m_mapping);
		CHECK_RET_CRASH(closed != 0);
	}
	if(m_file != INVALID_HANDLE_VALUE)
	{
		BOOL const closed = CloseHandle(m_file);
		CHECK_RET_CRASH(closed != 0);
	}
}

void mk::data_source_rommf_t::swap(data_source_rommf_t& other) noexcept
{
	using std::swap;
	swap(m_file, other.m_file);
	swap(m_size, other.m_size);
	swap(m_mapping, other.m_mapping);
	swap(m_view, other.m_view);
	swap(m_view_start, other.m_view_start);
	swap(m_position, other.m_position);
}

mk::data_source_rommf_t::operator bool() const
{
	return m_mapping != nullptr;
}

void mk::data_source_rommf_t::reset()
{
	*this = data_source_rommf_t{};
}


std::uint64_t mk::data_source_rommf_t::get_input_size() const
{
	return m_size;
}

std::uint64_t mk::data_source_rommf_t::get_input_position() const
{
	return m_position;
}

std::uint64_t mk::data_source_rommf_t::get_input_remaining_size() const
{
	return get_input_size() - get_input_position();
}

void const* mk::data_source_rommf_t::get_view() const
{
	std::uint64_t const file_remaining = m_size - m_view_start;
	int const view_size = file_remaining < detail::s_view_size ? static_cast<int>(file_remaining) : detail::s_view_size;
	assert(m_position >= m_view_start && m_position < m_view_start + view_size);
	std::uint64_t const offset_ = m_position - m_view_start;
	assert(offset_ <= view_size);
	std::size_t const offset = static_cast<std::size_t>(offset_);
	return static_cast<void const*>(static_cast<unsigned char const*>(m_view) + offset);
}

std::size_t mk::data_source_rommf_t::get_view_remaining_size() const
{
	std::uint64_t const file_remaining = m_size - m_view_start;
	int const view_size = file_remaining < detail::s_view_size ? static_cast<int>(file_remaining) : detail::s_view_size;
	assert(m_position >= m_view_start && m_position < m_view_start + view_size);
	std::uint64_t const offset_ = m_position - m_view_start;
	assert(offset_ <= view_size);
	std::size_t const offset = static_cast<std::size_t>(offset_);
	return view_size - offset;
}


void mk::data_source_rommf_t::consume(std::size_t const amount)
{
	assert(amount <= get_input_size());
	m_position += amount;
}

void mk::data_source_rommf_t::move_to(std::uint64_t const position, std::size_t const window_size)
{
	assert(position < get_input_size());
	assert(window_size <= detail::s_view_size - detail::s_view_granularity);

	std::uint64_t const file_remaining = m_size - m_view_start;
	int const view_size = file_remaining < detail::s_view_size ? static_cast<int>(file_remaining) : detail::s_view_size;

	std::uint64_t const view_end = m_view_start + view_size;
	std::uint64_t const position_end = position + window_size;

	if(position >= m_view_start && position_end <= view_end)
	{
		m_position = position;
	}
	else
	{
		if(m_view != nullptr)
		{
			BOOL const unmapped = UnmapViewOfFile(m_view);
			CHECK_RET_CRASH(unmapped != 0);
		}

		std::uint64_t const new_view_begin = position &~ std::uint64_t{detail::s_view_granularity - 1};
		std::uint32_t const new_view_begin_hi = static_cast<std::uint32_t>((new_view_begin >> (1 * 32)) & 0xFFFFFFFFull);
		std::uint32_t const new_view_begin_lo = static_cast<std::uint32_t>((new_view_begin >> (0 * 32)) & 0xFFFFFFFFull);
		std::uint64_t const new_file_remaining = m_size - new_view_begin;
		int const new_view_size = new_file_remaining < detail::s_view_size ? static_cast<int>(new_file_remaining) : detail::s_view_size;
		void const* const new_view = MapViewOfFile(m_mapping, FILE_MAP_READ, new_view_begin_hi, new_view_begin_lo, new_view_size);
		CHECK_RET_CRASH(new_view != nullptr);
		m_view = new_view;
		m_view_start = new_view_begin;
		m_position = position;
	}
}
