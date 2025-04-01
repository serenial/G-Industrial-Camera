#include "g_industrial_cam/device-io/buffer.hpp"

using namespace g_industrial_cam;

buffer::buffer(ArvBuffer* buf): m_buffer(buf){
    // nothing else to init;
}

buffer::~buffer(){

    g_clear_object(&m_buffer);
    m_buffer = nullptr;
}

bool buffer::is_valid() const{
    return m_buffer != nullptr;
}

bool buffer::has_status_success() const{
    return arv_buffer_get_status(m_buffer) == ARV_BUFFER_STATUS_SUCCESS;
}