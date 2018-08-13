/*
    printf.hpp
*/

#ifndef _PRINTF_HPP_
#define _PRINTF_HPP_

template<size_t S, typename... T>
void Printf(const TCHAR (&fmt)[S], T... args)
{
    _fputts(_T("native-msg-proxy "), stderr);
    _ftprintf(stderr, fmt, args...);
}

#endif