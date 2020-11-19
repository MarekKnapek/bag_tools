#include "read_only_file.h"

#include "utils.h"

#include <cassert>
#include <utility> // std::swap

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h> // close


mk::read_only_file_t::read_only_file_t() noexcept :
	m_fd(-1)
{
}

mk::read_only_file_t::read_only_file_t(char const* const& file_path) :
	mk::read_only_file_t()
{
	int const fd = open(file_path, O_RDONLY, O_CLOEXEC);
	CHECK_RET_V(fd != -1);
	assert(fd >= 0);
	m_fd = fd;
}

mk::read_only_file_t::read_only_file_t(mk::read_only_file_t&& other) noexcept :
	mk::read_only_file_t()
{
	swap(other);
}

mk::read_only_file_t& mk::read_only_file_t::operator=(mk::read_only_file_t&& other) noexcept
{
	swap(other);
	return *this;
}

mk::read_only_file_t::~read_only_file_t() noexcept
{
	if(!*this)
	{
		return;
	}
	int const closed = close(m_fd);
	CHECK_RET_V(closed == 0);
}

void mk::read_only_file_t::swap(mk::read_only_file_t& other) noexcept
{
	using std::swap;
	swap(m_fd, other.m_fd);
}

int const& mk::read_only_file_t::get() const
{
	return m_fd;
}

mk::read_only_file_t::operator bool() const
{
	return m_fd != -1;
}
