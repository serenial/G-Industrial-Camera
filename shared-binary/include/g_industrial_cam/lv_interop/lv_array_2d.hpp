#pragma once

#include <vector>
#include <array>
#include <functional>
#include <cstring>
#include <numeric>
#include <exception>
#include <algorithm>

#include "./lv_types.hpp"
#include "./lv_functions.hpp"
#include "./lv_array_md.hpp"

#include "./set_packing.hpp"

namespace namespace g_industrial_cam
{
    namespace lv_interop
    {

        template <class T>
        class LV_2DArrayHandle_t : public LV_MDArrayHandle_t<2,T>
        {
        public:
            LV_2DArrayHandle_t() = delete;

            cv::Size size() const
            {
                auto dims = this->extents();
                return cv::Size(dims[1], dims[0]);
            }
        };
    }
}

#include "./reset_packing.hpp"