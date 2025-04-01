#include <arv.h>

#include "g_industrial_cam/device-io/camera.hpp"
#include "g_industrial_cam/device-io/buffer.hpp"
#include "g_industrial_cam/device-io/aravis-error.hpp"

using namespace g_industrial_cam;

camera::camera(const std::string& identifier_utf8) : m_camera(nullptr),
                                         m_is_streaming(false)
{
    aravis_error err;
    m_camera = arv_camera_new(identifier_utf8.c_str(), err);
    aravis_error::check_error(err);
}

camera::~camera()
{
    if(m_camera!=nullptr){
        g_clear_object(&m_camera);
    }
    m_camera = nullptr;
}

buffer *camera::take_snapshot(uint32_t timeout) const
{
    aravis_error err;

    /* Acquire a single buffer */
    ArvBuffer *buf = arv_camera_acquisition(m_camera, timeout, err);

    aravis_error::check_error(err);

    return new buffer(buf);
}