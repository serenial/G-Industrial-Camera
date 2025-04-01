#pragma once

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
    private:
        ArvBuffer *m_buffer;
    };
}