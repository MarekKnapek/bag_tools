#pragma once


#include <utility> // std::forward


namespace mk
{


	template<typename...>
	struct overload_t
	{
	};

	template<typename head_t, typename... tail_ts>
	struct overload_t<head_t, tail_ts...> : head_t, overload_t<tail_ts...>
	{
		template<typename u, typename... us>
		overload_t(u&& obj, us&&... objs) :
			head_t(std::forward<u>(obj)),
			overload_t<tail_ts...>(std::forward<us>(objs)...)
		{
		}
		using head_t::operator();
		using overload_t<tail_ts...>::operator();
	};

	template<typename tail_t>
	struct overload_t<tail_t> : tail_t
	{
		template<typename u>
		overload_t(u&& obj) :
			tail_t(std::forward<u>(obj))
		{
		}
		using tail_t::operator();
	};

	template<typename... ts>
	overload_t<ts...> make_overload(ts&&... objs)
	{
		return overload_t<ts...>{std::forward<ts>(objs)...};
	}


}
