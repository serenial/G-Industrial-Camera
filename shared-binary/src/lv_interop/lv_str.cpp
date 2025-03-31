#ifdef WIN32
// for UTF8 to acii conversion on windows
#include <cstring>
#include <Windows.h>
#endif

#include "g_industrial_cam/lv_interop/lv_str.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

LV_StringHandle_t::operator std::string_view() const
{
    return std::string_view{begin(), size()};
}

LV_StringHandle_t::operator const std::string() const
{
    return std::string{begin(), size()};
}

void LV_StringHandle_t::copy_from(const std::string &string){
    size_to_fit(string.length());
    std::memcpy(begin(), string.data(), string.length());
}

void LV_StringHandle_t::copy_from_utf8(const char* utf8_chars){

    #ifndef WIN32
    // not windows - just call copy_from
        copy_from(utf8_chars);
    #else
    // first convert from UTF8 to wide char
    // get size of wstring
    auto n_bytes_wide_string = MultiByteToWideChar(CP_UTF8, 0, utf8_chars, static_cast<int>(std::strlen(utf8_chars)), NULL ,0);

    if(n_bytes_wide_string <=0){
        // the widestring would have zero length so size_to_fit and return
        size_to_fit(0);
        return;
    }
    // convert to wide-string
    std::wstring wide(n_bytes_wide_string,0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_chars, static_cast<int>(std::strlen(utf8_chars)), &wide[0], n_bytes_wide_string);

    // convert wide-string to ANSI
    auto n_bytes_ascii = WideCharToMultiByte(CP_ACP, 0, &wide[0], n_bytes_wide_string, NULL, 0, NULL, NULL);

    // resize string handle
    size_to_fit(n_bytes_ascii);

    if(n_bytes_ascii == 0){
        return;
    }

    // copy ANSI into the string handle
    WideCharToMultiByte(CP_ACP, 0, &wide[0], n_bytes_wide_string, begin(), n_bytes_ascii, NULL, NULL);

    #endif
}