#include <memory>
#include <functional>
#include <utility>
#include <thread>

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

camera::camera(camera::signal_fn_t on_disconnect) : m_on_disconnect(on_disconnect)
{
    // nothing else to init;
}

camera *camera::create(const std::string &identifier_utf8, camera::signal_fn_t on_disconnect)
{
    auto c = new camera(on_disconnect);
    c->connect(identifier_utf8);
    return c;
}

void camera::connect(const std::string &identifier_utf8)
{
    aravis_error err;
    m_camera = arv_camera_new(identifier_utf8.c_str(), err);
    aravis_error::check_error(err);

    // connect on_disconnect caller
    g_signal_connect(arv_camera_get_device (m_camera), "control-lost", G_CALLBACK(control_lost), this);

    // get payload
    m_camera_payload = arv_camera_get_payload(m_camera, err);
    aravis_error::check_error(err);
}

camera::~camera()
{
    try
    {
        if (m_stream)
        {
            stream_stop();
        }
    }
    catch (...)
    {
        // do nothing
    }
    g_clear_object(&m_camera);
    m_camera = nullptr;
}


void camera::control_lost(ArvGvDevice *gv_device, void* self_void_ptr){

    auto self = static_cast<camera*>(self_void_ptr);

    self->m_on_disconnect();
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

ArvBuffer *camera::take_snapshot(int32_t timeout_ms) const
{
    aravis_error err;

    // make timeout behvaiour match LabVIEW-style timeout-symantics

    uint64_t timeout_us = timeout_ms * 1000;

    if (timeout_ms == 0)
    {
        timeout_us = 1; // the closest we can get to zero ms timeout is 1us;
    }

    if (timeout_ms < 0)
    {
        timeout_us = 0;
    }

    /* Acquire a single buffer */
    ArvBuffer *buf = arv_camera_acquisition(m_camera, timeout_us, err);

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

void camera::stream_start(uint16_t n_additional_buffers, camera::signal_fn_t on_stream_start, camera::signal_fn_t on_stream_stop)
{

    if (m_stream != nullptr)
    {
        throw std::runtime_error("Camera is already streaming.");
    }

    aravis_error err;

    arv_camera_set_acquisition_mode(m_camera, ARV_ACQUISITION_MODE_CONTINUOUS, err);

    aravis_error::check_error(err);

    m_on_stream_start = on_stream_start;
    m_on_stream_stop = on_stream_stop;

    m_stream = arv_camera_create_stream(m_camera, &camera::stream_callback, this, err);

    if (!ARV_IS_STREAM(m_stream))
    {
        m_stream = nullptr;
        throw std::runtime_error("Unable to create stream");
    }

    for (int i = 0; i < n_additional_buffers + 2; i++)
    {
        // queue up the buffers into the stream - enough to fill the circular_buffer and one extra to load into the stream fifo
        arv_stream_push_buffer(m_stream, arv_buffer_new_allocate(m_camera_payload));
    }

    // we are going to use a circular buffer to automatically push buffers from the output stream FIFO
    // into the input stream FIFO
    m_stream_buffers.set_capacity(n_additional_buffers + 1);

    arv_camera_start_acquisition(m_camera, err);

    aravis_error::check_error(err);
}

void camera::stream_stop()
{

    if (!m_stream)
    {
        return;
    }

    aravis_error err;

    arv_camera_stop_acquisition(m_camera, err);

    g_clear_object(&m_stream);
    m_stream = nullptr;

    // cleanup any buffers in the circular buffer
    for (auto buf : m_stream_buffers)
    {
        g_object_unref(buf);
    }

    m_stream_buffers.clear();

    aravis_error::check_error(err);
}

void camera::stream_pop_buffer(int32_t timeout_ms, ArvBuffer **buffer_ptr, camera::signal_fn_t on_stream_capture_success, camera::signal_fn_t on_stream_capture_fail)
{

    if (!buffer_ptr)
    {
        throw std::invalid_argument("Buffer Pointer cannot be null.");
    }

    if (!m_stream)
    {
        throw std::runtime_error("Camera Stream is not running.");
    }

    std::thread([&, on_stream_capture_success, on_stream_capture_fail]
                {
                    auto check_something_to_pop = [&]
                    { return !m_stream_buffers.empty() || m_stream == nullptr; };
                    bool success = true;

                    try{

                    std::unique_lock lk(m_stream_buffers_mtx);

                    if (timeout_ms < 0)
                    {
                        m_stream_event.wait(lk, check_something_to_pop);
                    }
                    else
                    {
                        success = m_stream_event.wait_for(lk, std::chrono::milliseconds(timeout_ms), check_something_to_pop);
                    }

                    if (success && !m_stream_buffers.empty())
                    {
                        // something to get from the circular buffer

                        ArvBuffer *to_push_fifo = *buffer_ptr;

                        // check buffer we were passed
                        if (*buffer_ptr)
                        {
                            // check buffer is correct size - delete and reallocate if not
                            size_t passed_buffer_size;
                            arv_buffer_get_image_data(*buffer_ptr, &passed_buffer_size);
                            if (passed_buffer_size != m_camera_payload)
                            {
                                g_object_unref(*buffer_ptr);
                                to_push_fifo = arv_buffer_new_allocate(m_camera_payload);
                            }
                        }
                        else
                        {
                            // null-buffer: create new
                            to_push_fifo = arv_buffer_new_allocate(m_camera_payload);
                        }

                        to_push_fifo = *buffer_ptr;

                        // collect an old buffer from the circular buffer;
                        *buffer_ptr = m_stream_buffers.front();

                        // pop the front to remove the buffer we have just grabbed.
                        m_stream_buffers.pop_front();

                        // push this new buffer onto the stream FIFO
                        arv_stream_push_buffer(m_stream, to_push_fifo);
                        on_stream_capture_success();
                    }
                    else{
                        on_stream_capture_fail();
                    }

                    lk.unlock();
                }
                catch(...){
                    on_stream_capture_fail();
                } })
        .detach();
}

void camera::stream_callback(void *self_void_ptr, ArvStreamCallbackType type, ArvBuffer *buffer)
{

    auto self = static_cast<camera *>(self_void_ptr);

    switch (type)
    {
    case ARV_STREAM_CALLBACK_TYPE_INIT:
        self->m_on_stream_start();
        break;
    case ARV_STREAM_CALLBACK_TYPE_START_BUFFER:
        break;
    case ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE:

        if (buffer != arv_stream_pop_buffer(self->m_stream) || buffer == nullptr)
        {
            return;
        }

        if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS)
        {

            {
                std::lock_guard lk(self->m_stream_buffers_mtx);

                if (self->m_stream_buffers.full())
                {
                    // move the element that is about to be overwritten into the input FIFO
                    arv_stream_push_buffer(self->m_stream, self->m_stream_buffers.front());
                }

                // take the newly filled buffer and add it to the end of the circular buffer
                self->m_stream_buffers.push_back(buffer);

                // mutex'd work done
            }

            self->m_stream_event.notify_one();
        }

        break;
    case ARV_STREAM_CALLBACK_TYPE_EXIT:
        self->m_on_stream_stop();
        /* Stream thread ended */
        break;
    }
}