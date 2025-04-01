#include "g_industrial_cam/device-io/buffer.hpp"

using namespace g_industrial_cam;

buffer::buffer(ArvBuffer *buf) : m_buffer(buf)
{
}

buffer::~buffer()
{
    g_clear_object(&m_buffer);
    m_buffer = nullptr;
}

bool buffer::is_valid() const
{
    return m_buffer != nullptr;
}

bool buffer::has_status_success() const
{
    return arv_buffer_get_status(m_buffer) == ARV_BUFFER_STATUS_SUCCESS;
}

const uint8_t *buffer::begin() const
{
    return buffer_image_data().first;
}

const uint8_t *buffer::end() const
{
    auto image_data = buffer_image_data();
    return image_data.first + image_data.second;
}

const size_t buffer::size() const
{
    return buffer_image_data().second;
}

std::pair<const uint8_t *, const size_t> buffer::buffer_image_data() const
{
    size_t size;
    auto begin = static_cast<const uint8_t *>(arv_buffer_get_image_data(m_buffer, &size));
    return std::make_pair(begin, size);
}

const uint16_t buffer::width() const{
    return arv_buffer_get_image_width(m_buffer);
}

const uint16_t buffer::height() const{
    return arv_buffer_get_image_height(m_buffer);
}