#pragma once

#include <string>
#include <vector>

#include <arv.h>

#include "./buffer.hpp"

namespace g_industrial_cam{
    class camera{
        public:
        camera() = delete;
        camera(const std::string& identifier_utf8);
        ~camera();
        buffer* take_snapshot(uint32_t timeout) const;
        void get_avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        private:
        ArvCamera *m_camera;
        bool m_is_streaming;
    };
}