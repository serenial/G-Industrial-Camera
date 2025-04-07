#include <memory>

#include <arv.h>

#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/device-io/camera.hpp"
#include "g_industrial_cam/device-io/aravis-error.hpp"

namespace
{
    template <class T>
    struct g_pointer_deleter_t
    {
        void operator()(T ptr)
        {
            g_free(ptr);
        }
    };
}

using namespace g_industrial_cam;

camera::camera(const std::string &identifier_utf8) : m_camera(nullptr),
                                                     m_is_streaming(false)
{
    aravis_error err;
    m_camera = arv_camera_new(identifier_utf8.c_str(), err);
    aravis_error::check_error(err);
}

camera::~camera()
{
    g_clear_object(&m_camera);
    m_camera = nullptr;
}

void camera::get_avaliable_pixel_formats(std::vector<std::string> &pixel_formats_utf8) const
{

    guint n_formats = 0;
    aravis_error err;

    // use a unique_ptr to ensure the const char** "container" returned by arv_camera_dup_avaliable_pixel_formats_as_strings
    // is cleared - we own the container but not the data
    std::unique_ptr<const char *, ::g_pointer_deleter_t<const char **>> list(arv_camera_dup_available_pixel_formats_as_strings(m_camera, &n_formats, err));

    aravis_error::check_error(err);

    for (guint i = 0; i < n_formats; i++)
    {
        pixel_formats_utf8.emplace_back(list.get()[i]);
    }
}

ArvBuffer *camera::take_snapshot(uint64_t timeout) const
{
    aravis_error err;

    /* Acquire a single buffer */
    ArvBuffer *buf = arv_camera_acquisition(m_camera, timeout, err);

    aravis_error::check_error(err);

    return buf;
}

std::string camera::get_pixel_format() const
{
    aravis_error err;

    auto format = std::string(arv_camera_get_pixel_format_as_string(m_camera, err));

    aravis_error::check_error(err);

    return format;
}

void camera::set_pixel_format(const std::string &pixel_format_utf8)
{

    aravis_error err;

    arv_camera_set_pixel_format_from_string(m_camera, pixel_format_utf8.c_str(), err);

    aravis_error::check_error(err);
}