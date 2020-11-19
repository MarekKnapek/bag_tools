#pragma once


namespace mk
{


	class read_only_file_t
	{
	public:
		read_only_file_t() noexcept;
		read_only_file_t(char const* const& file_path);
		read_only_file_t(mk::read_only_file_t const&) = delete;
		read_only_file_t(mk::read_only_file_t&& other) noexcept;
		mk::read_only_file_t& operator=(mk::read_only_file_t const&) = delete;
		mk::read_only_file_t& operator=(mk::read_only_file_t&& other) noexcept;
		~read_only_file_t() noexcept;
		void swap(mk::read_only_file_t& other) noexcept;
	public:
		int const& get() const;
		explicit operator bool() const;
	private:
		int m_fd;
	};

	inline void swap(mk::read_only_file_t& a, mk::read_only_file_t& b) noexcept { a.swap(b); }


}
