#include "data_source_mem.h"

#include <cassert>
#include <utility> // std::swap


mk::data_source_mem_t::data_source_mem_t() noexcept :
	m_data(),
	m_size(),
	m_position()
{
}

mk::data_source_mem_t mk::data_source_mem_t::make(void const* const data, std::size_t const size)
{
	data_source_mem_t source;
	source.m_data = data;
	source.m_size = size;
	source.m_position = 0;
	return source;
}

mk::data_source_mem_t::data_source_mem_t(data_source_mem_t&& other) noexcept :
	data_source_mem_t()
{
	swap(other);
}

mk::data_source_mem_t& mk::data_source_mem_t::operator=(data_source_mem_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::data_source_mem_t::~data_source_mem_t() noexcept
{
}

void mk::data_source_mem_t::swap(data_source_mem_t& other) noexcept
{
	using std::swap;
	swap(m_data, other.m_data);
	swap(m_size, other.m_size);
	swap(m_position, other.m_position);
}

mk::data_source_mem_t::operator bool() const
{
	return m_data != nullptr;
}

void mk::data_source_mem_t::reset()
{
	*this = data_source_mem_t{};
}


std::uint64_t mk::data_source_mem_t::get_input_size() const
{
	return m_size;
}

std::uint64_t mk::data_source_mem_t::get_input_position() const
{
	return m_position;
}

std::uint64_t mk::data_source_mem_t::get_input_remaining_size() const
{
	return get_input_size() - get_input_position();
}

void const* mk::data_source_mem_t::get_view() const
{
	return static_cast<void const*>(static_cast<unsigned char const*>(m_data) + m_position);
}

std::size_t mk::data_source_mem_t::get_view_remaining_size() const
{
	return m_size - m_position;
}


void mk::data_source_mem_t::consume(std::size_t const amount)
{
	assert(amount <= get_view_remaining_size());
	m_position += amount;
}

void mk::data_source_mem_t::move_to(std::uint64_t const position, [[maybe_unused]] std::size_t const window_size)
{
	assert(position < m_size);
	assert(window_size <= m_size);
	m_position = static_cast<std::size_t>(position);
}
