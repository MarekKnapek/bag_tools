#pragma once


#define CHECK_RET(X, R) do{ if(X){}else{ mk::check_ret_failed(__FILE__, static_cast<int>(__LINE__), #X); return R; } }while(false)
#define CHECK_RET_V(X) do{ if(X){}else{ mk::check_ret_failed(__FILE__, static_cast<int>(__LINE__), #X); return; } }while(false)
#define CHECK_RET_F(X) do{ if(X){}else{ mk::check_ret_failed(__FILE__, static_cast<int>(__LINE__), #X); return false; } }while(false)
#define CHECK_RET_CRASH(X) do{ if(X){}else{ mk::check_ret_failed(__FILE__, static_cast<int>(__LINE__), #X); int volatile* volatile ptr = nullptr; *ptr = 0; } }while(false)


namespace mk
{


	void check_ret_failed(char const* const& file, int const& line, char const* const& expr);


}
