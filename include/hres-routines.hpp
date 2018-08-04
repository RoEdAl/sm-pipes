/*
    hres-routines.hpp
*/

#ifndef _HRES_ROUTINES_HPP_
#define _HRES_ROUTINES_HPP_

namespace hres_routines
{
    template<DWORD error_code>
    bool win32_error(HRESULT hRes)
    {
        return hRes == HRESULT_FROM_WIN32(error_code);
    }

    bool more_data(HRESULT hRes)
    {
        return win32_error<ERROR_MORE_DATA>(hRes);
    }

    bool read_succeeded(HRESULT hRes)
    {
        return (SUCCEEDED(hRes) || more_data(hRes));
    }

    bool abort_succeeded(HRESULT hRes)
    {
        return (SUCCEEDED(hRes) || win32_error<ERROR_OPERATION_ABORTED>(hRes));
    }
};

#endif // _HRES_ROUTINES_HPP_
