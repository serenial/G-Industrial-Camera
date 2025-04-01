#if defined(_WIN32)
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

void LV_StringHandle_t::copy_from(const uint8_t* data, size_t size){
    size_to_fit(size);
    std::memcpy(begin(), data,size);
}

void LV_StringHandle_t::copy_from(const std::string& str){
    copy_from(reinterpret_cast<const uint8_t*>(&(str[0])), str.length());
}

void LV_StringHandle_t::copy_from_utf8(const char* utf8_chars){

    #ifndef _WIN32
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
    auto n_bytes_ansi = WideCharToMultiByte(CP_ACP, 0, &wide[0], n_bytes_wide_string, NULL, 0, NULL, NULL);

    // resize string handle
    size_to_fit(n_bytes_ansi);

    if(n_bytes_ansi == 0){
        return;
    }

    // copy ANSI into the string handle
    WideCharToMultiByte(CP_ACP, 0, &wide[0], n_bytes_wide_string, begin(), n_bytes_ansi, NULL, NULL);

    #endif
}

void LV_StringHandle_t::copy_from_utf8(const std::string& utf8string){
    copy_from_utf8(utf8string.c_str());
}

std::string LV_StringHandle_t::to_utf8_string() const{
    #ifndef WIN32
    // not windows - just convert to std::string
    return static_cast<std::string>(*this);
    #else
    // first convert from ANSI to wide char
    // get size of wstring
    auto n_bytes_wide_string = MultiByteToWideChar(CP_ACP, 0, begin(), static_cast<int>(size()), NULL ,0);

    if(n_bytes_wide_string <=0){
        // the widestring would have zero length so size_to_fit and return
        return "";
    }

    // convert to wide-string
    std::wstring wide(n_bytes_wide_string,0);
    MultiByteToWideChar(CP_ACP, 0, begin(), static_cast<int>(size()), &wide[0], n_bytes_wide_string);

    // convert wide-string to UTF8
    auto n_bytes_utf8 = WideCharToMultiByte(CP_UTF8, 0, &wide[0], n_bytes_wide_string, NULL, 0, NULL, NULL);

    std::string output(n_bytes_utf8,0);

    // copy ANSI into the string handle
    WideCharToMultiByte(CP_UTF8, 0, &wide[0], n_bytes_wide_string, &output[0], n_bytes_utf8 , NULL, NULL);

    return output;

    #endif
}