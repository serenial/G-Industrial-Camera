#pragma once

#include <string_view>

#include <arv.h>

#include "./lv_interop/lv_str.hpp"
#include "./buffer.hpp"

namespace g_industrial_cam{
    class camera{
        public:
        camera() = delete;
        camera(const std::string& identifier);
        ~camera();
        buffer* take_snapshot(uint32_t timeout) const;
        private:
        ArvCamera *m_camera;
        bool m_is_streaming;
    };
}