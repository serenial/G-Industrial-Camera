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