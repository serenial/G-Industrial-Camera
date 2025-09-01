#include "g_industrial_cam/lv_interop/lv_str_array_1d.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

void LV_1DStringArrayHandle_t::copy_from_utf8(const std::vector<std::string> &vec){
    
    size_to_fit(vec.size());
    copy_element_by_element_from(vec, [](auto from, auto to){ to->copy_from_utf8(from); });
}
