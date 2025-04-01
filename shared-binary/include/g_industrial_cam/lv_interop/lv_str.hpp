#pragma once

#include <string>
#include <string_view>

#include "./lv_types.hpp"
#include "./lv_array_1d.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        class LV_StringHandle_t : public LV_1DArrayHandle_t<char>
        {
        public:
            operator std::string_view() const;
            operator const std::string() const;
            void copy_from(const std::string &);
            void copy_from_utf8(const std::string &);
            void copy_from_utf8(const char *);
            std::string to_utf8_string() const;
        };
    }
}