#pragma once

#include <string>
#include <vector>

#include <arv.h>

namespace g_industrial_cam{
    class camera{
        public:
         // two stage intializer as LabVIEW aborts on a failed constructor
        static camera* create(const std::string& identifier_utf8);
        ~camera();
        ArvBuffer* take_snapshot(uint64_t timeout) const;
        void get_avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        private:
        camera() = default;
        void connect(const std::string& identifier_utf8);
        ArvCamera *m_camera;
        bool m_is_streaming;
    };
}