#pragma once

#include <utility>

#include <arv.h>

namespace g_industrial_cam
{
    class buffer
    {
    public:
        buffer() = delete;
        buffer(ArvBuffer *buf);
        ~buffer();
        bool is_valid() const;
        bool has_status_success() const;
        const uint8_t *begin() const;
        const uint8_t *end() const;
        const size_t size() const;
        const uint16_t width() const;
        const uint16_t height() const;

    private:
        ArvBuffer *m_buffer;
        std::pair<const uint8_t *, const size_t> buffer_image_data() const;
    };
}