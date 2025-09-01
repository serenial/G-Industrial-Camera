#pragma once

#include <string>
#include <vector>

#include "./lv_array_1d.hpp"
#include "./lv_str.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        class LV_1DStringArrayHandle_t : public LV_1DArrayHandle_t<LV_StringHandle_t>
        {
        public:
            void copy_from_utf8(const std::vector<std::string> &vec);
        };
    }
}