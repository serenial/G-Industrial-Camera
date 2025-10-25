#include <memory>
#include <functional>
#include <utility>
#include <thread>
#include <type_traits>

#include <arv.h>

#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/device-io/camera.hpp"
#include "g_industrial_cam/device-io/aravis-error.hpp"

namespace
{
    template <class T>
    struct deleter_for_g_pointer
    {
        void operator()(T ptr)
        {
            g_free(ptr);
        }
    };
}

using namespace g_industrial_cam;

camera::camera(camera::signal_fn_t on_disconnect) : m_gv_buffer_size(0),
                                                    m_callback_on_disconnect(on_disconnect),
                                                    m_stream(nullptr)
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
    aravis_error::check(err);

    // connect on_disconnect caller for gige vision cameras
    if (arv_camera_is_gv_device(m_camera))
    {
        g_signal_connect(arv_camera_get_device(m_camera), "control-lost", G_CALLBACK(gv_control_lost_callback), this);
    }

    aravis_error::check(err);
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
    g_object_unref(m_camera);
    m_camera = nullptr;
}

void camera::gv_control_lost_callback(ArvGvDevice *gv_device, camera *self)
{
    if (!self)
    {
        return;
    }
    self->m_callback_on_disconnect();
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

    aravis_error::check(err);

    return buf;
}

camera::timeout_result camera::stream_start(int32_t max_sequential_errors, uint16_t n_additional_buffers, int32_t timeout_ms, bool fixed_num_frames, camera::signal_fn_t on_stream_errors_exceeded, camera::signal_fn_t on_stream_stop)
{

    if (m_stream != nullptr)
    {
        throw std::runtime_error("Camera is already streaming.");
    }

    aravis_error err;

    arv_camera_set_acquisition_mode(m_camera, fixed_num_frames ? ARV_ACQUISITION_MODE_MULTI_FRAME : ARV_ACQUISITION_MODE_CONTINUOUS, err);

    aravis_error::check(err);

    // init internal class members to get ready to stream
    m_callback_on_stream_error_limit_exceeded = on_stream_errors_exceeded;
    m_callback_on_stream_stop = on_stream_stop;
    m_stream_max_sequential_errors = max_sequential_errors;
    m_stream_started = false;

    if (arv_camera_is_gv_device(m_camera))
    {

        g_object_set(m_stream,
                     "socket-buffer", m_gv_buffer_size > 0 ? ARV_GV_STREAM_SOCKET_BUFFER_FIXED : ARV_GV_STREAM_SOCKET_BUFFER_AUTO,
                     "socket-buffer-size", m_gv_buffer_size,
                     NULL);
    }

    // start the stream

    m_stream = arv_camera_create_stream(m_camera, &camera::stream_event_callback, this, err);

    if (!ARV_IS_STREAM(m_stream))
    {
        m_stream = nullptr;
        throw std::runtime_error("Unable to create stream");
    }

    // get payload
    m_camera_payload = arv_camera_get_payload(m_camera, err);

    for (int i = 0; i < n_additional_buffers; i++)
    {
        // queue up the buffers into the stream - enough to fill the circular_buffer and one extra to load into the stream fifo
        arv_stream_push_buffer(m_stream, arv_buffer_new_allocate(m_camera_payload));
    }

    // we are going to use a circular buffer to automatically push buffers from the output stream FIFO
    // into the input stream FIFO
    m_stream_buffers.set_capacity(n_additional_buffers + 1);

    arv_camera_start_acquisition(m_camera, err);

    aravis_error::check(err);

    // connect the new buffer callback
    g_signal_connect(m_stream, "new-buffer", G_CALLBACK(stream_buffer_callback), this);
    arv_stream_set_emit_signals(m_stream, true);

    bool no_timeout = true;

    auto started = [&]()
    { return m_stream_started; };

    std::unique_lock lk(m_stream_buffers_mtx);

    if (timeout_ms < 0)
    {
        m_stream_event.wait(lk, started);
    }
    else
    {
        no_timeout = m_stream_event.wait_for(lk, std::chrono::milliseconds(timeout_ms), started);
    }

    return no_timeout ? camera::timeout_result::success : camera::timeout_result::timeout;
}

void camera::stream_stop()
{

    if (!m_stream)
    {
        return;
    }

    arv_stream_set_emit_signals(m_stream, false); // stop the new buffer signal

    aravis_error err;

    arv_camera_stop_acquisition(m_camera, err);

    g_object_unref(m_stream);
    m_stream = nullptr;

    // cleanup any buffers in the circular buffer
    for (auto buf : m_stream_buffers)
    {
        g_object_unref(buf);
    }

    m_stream_buffers.clear();

    aravis_error::check(err);
}

camera::timeout_result camera::stream_pop_buffer(int32_t timeout_ms, ArvBuffer **buffer_ptr)
{

    if (!buffer_ptr)
    {
        throw std::invalid_argument("Buffer Pointer cannot be null.");
    }

    if (!m_stream)
    {
        throw std::runtime_error("Camera Stream is not running.");
    }

    // check buffer we were passed
    if (*buffer_ptr)
    {
        // check buffer is correct size - delete and reallocate if not
        size_t passed_buffer_size;
        arv_buffer_get_image_data(*buffer_ptr, &passed_buffer_size);
        if (passed_buffer_size != m_camera_payload)
        {
            g_object_unref(*buffer_ptr);
            *buffer_ptr = arv_buffer_new_allocate(m_camera_payload);
        }
    }
    else
    {
        // null-buffer: create new
        *buffer_ptr = arv_buffer_new_allocate(m_camera_payload);
    }

    // push this into the stream FIFO
    arv_stream_push_buffer(m_stream, *buffer_ptr);

    // the buffer belongs to the camera now
    *buffer_ptr = nullptr;

    auto check_something_to_pop = [&]
    { return !m_stream_buffers.empty() || m_stream == nullptr; };

    bool no_timeout = true;

    std::unique_lock lk(m_stream_buffers_mtx);

    if (timeout_ms < 0)
    {
        m_stream_event.wait(lk, check_something_to_pop);
    }
    else
    {
        no_timeout = m_stream_event.wait_for(lk, std::chrono::milliseconds(timeout_ms), check_something_to_pop);
    }

    if (no_timeout && !m_stream_buffers.empty())
    {
        // something to get from the circular buffer
        // collect the oldest buffer from the circular buffer;
        *buffer_ptr = m_stream_buffers.front();

        // pop the front to remove the buffer we have just grabbed.
        m_stream_buffers.pop_front();
    }

    lk.unlock();

    return no_timeout ? timeout_result::success : timeout_result::timeout;
}

void camera::stream_event_callback(void *self_void_ptr, ArvStreamCallbackType type, ArvBuffer *buffer)
{
    auto self = static_cast<camera *>(self_void_ptr);

    if (!self)
    {
        return;
    }

    switch (type)
    {
    case ARV_STREAM_CALLBACK_TYPE_INIT:
    {
        std::lock_guard lk(self->m_stream_buffers_mtx);
        self->m_stream_sequential_error_count = 0;
        self->m_stream_started = true;
        self->m_stream_frames_error_count = 0;
        self->m_stream_frames_count = 0;
    }
        self->m_stream_event.notify_one();
        break;
    case ARV_STREAM_CALLBACK_TYPE_EXIT:
        self->m_callback_on_stream_stop();
        /* Stream thread ended */
        break;
    }
}

void camera::stream_buffer_callback(ArvStream *stream, camera *self)
{

    ArvBuffer *buffer = arv_stream_pop_buffer(stream);
    self->m_stream_frames_count++;

    if (arv_buffer_get_status(buffer) == ARV_BUFFER_STATUS_SUCCESS)
    {

        {
            std::lock_guard lk(self->m_stream_buffers_mtx);

            if (self->m_stream_buffers.full())
            {
                if (self->m_stream_buffers.capacity() == 1)
                {
                    // unref the object that is about to be overwritten to avoid a memory leak
                    g_object_unref(self->m_stream_buffers.front());
                }
                else
                {
                    // move the element that is about to be overwritten into the input FIFO
                    arv_stream_push_buffer(stream, self->m_stream_buffers.front());
                }
            }

            // take the newly filled buffer and add it to the end of the circular buffer
            self->m_stream_buffers.push_back(buffer);
            self->m_stream_sequential_error_count = 0;
            // mutex'd work done
        }
        self->m_stream_event.notify_one();
    }
    else
    {
        // just push this buffer back into the device
        arv_stream_push_buffer(stream, buffer);
        self->m_stream_frames_error_count++;
        if (self->m_stream_max_sequential_errors >= 0)
        {
            self->m_stream_sequential_error_count++;
            if (self->m_stream_sequential_error_count > self->m_stream_max_sequential_errors)
            {
                self->m_callback_on_stream_error_limit_exceeded();
            }
        }
    }
}

void camera::set_pixel_format(const std::string &pixel_format_utf8)
{
    call_camera_fn_with_no_return(arv_camera_set_pixel_format_from_string, pixel_format_utf8.c_str());
}

void camera::clear_triggers()
{
    call_camera_fn_with_no_return(arv_camera_clear_triggers);
}

std::string camera::get_trigger_source() const
{
    return call_camera_fn(arv_camera_get_trigger_source);
}

bool camera::is_enumeration_entry_available(const std::string &feature_utf8, const std::string &entry_utf8) const
{
    return call_camera_fn(arv_camera_is_enumeration_entry_available, feature_utf8.c_str(), entry_utf8.c_str());
}

bool camera::is_feature_available(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_is_feature_available, feature_utf8.c_str());
}

bool camera::is_feature_implemented(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_is_feature_implemented, feature_utf8.c_str());
}

bool camera::is_software_trigger_supported() const
{
    return call_camera_fn(arv_camera_is_software_trigger_supported);
}

void camera::set_boolean(const std::string &feature_utf8, bool value) const
{

    call_camera_fn_with_no_return(arv_camera_set_boolean, feature_utf8.c_str(), value);
}

void camera::set_trigger(const std::string &source_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_set_trigger, source_utf8.c_str());
}

void camera::set_trigger_source(const std::string &source_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_set_trigger_source, source_utf8.c_str());
}

void camera::software_trigger() const
{
    call_camera_fn_with_no_return(arv_camera_software_trigger);
}

std::string camera::get_string(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_string, feature_utf8.c_str());
}

void camera::set_string(const std::string &feature_utf8, const std::string &value_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_set_string, feature_utf8.c_str(), value_utf8.c_str());
}

void camera::available_black_levels(std::vector<std::string> &black_levels_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_black_levels, black_levels_utf8);
}

void camera::available_components(std::vector<std::string> &components_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_components, components_utf8);
}

void camera::available_enumerations(const std::string &feature_utf, std::vector<std::string> &enums_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_enumerations_as_strings, enums_utf8, feature_utf.c_str());
}

void camera::available_gains(std::vector<std::string> &gains_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_gains, gains_utf8);
}

void camera::available_trigger_sources(std::vector<std::string> &trigger_sources_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_trigger_sources, trigger_sources_utf8);
}

void camera::available_pixel_formats(std::vector<std::string> &pixel_formats_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_pixel_formats_as_strings, pixel_formats_utf8);
}

void camera::available_triggers(std::vector<std::string> &triggers_utf8) const
{
    call_camera_fn_to_populate_list(arv_camera_dup_available_triggers, triggers_utf8);
}

void camera::read_register(const std::string &register_utf8, std::vector<std::byte> &bytes) const
{
    guint64 n_bytes = 0;
    aravis_error err;
    std::unique_ptr<void, ::deleter_for_g_pointer<void *>> result(arv_camera_dup_register(m_camera, register_utf8.c_str(), &n_bytes, err));

    bytes.resize(n_bytes);

    std::memcpy(bytes.data(), result.get(), n_bytes);
}

void camera::execute_command(const std::string &command_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_execute_command, command_utf8.c_str());
}

bool camera::get_boolean(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_boolean, feature_utf8.c_str());
}

double camera::get_exposure_time() const
{
    return call_camera_fn(arv_camera_get_exposure_time);
}

camera::auto_mode camera::get_exposure_time_auto() const
{
    return static_cast<auto_mode>(call_camera_fn(arv_camera_get_exposure_time_auto));
}

camera::bounds_t<double> camera::get_exposure_time_bounds() const
{
    return call_camera_fn_to_get_bounds<double>(arv_camera_get_exposure_time_bounds);
}

camera::representation camera::get_exposure_time_representation() const
{
    return static_cast<representation>(arv_camera_get_exposure_time_representation(m_camera));
}

double camera::get_float(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_float, feature_utf8.c_str());
}

camera::bounds_t<double> camera::get_float_bounds(const std::string &feature_utf8) const
{
    return call_camera_fn_to_get_bounds<double>(arv_camera_get_float_bounds, feature_utf8.c_str());
}

camera::representation camera::get_feature_representation(const std::string &feature_utf8) const
{
    return static_cast<representation>(arv_camera_get_feature_representation(m_camera, feature_utf8.c_str()));
}

double camera::get_float_increment(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_float_increment, feature_utf8.c_str());
}

double camera::get_frame_rate() const
{
    return call_camera_fn(arv_camera_get_frame_rate);
}

camera::bounds_t<double> camera::get_frame_rate_bounds() const
{
    return call_camera_fn_to_get_bounds<double>(arv_camera_get_frame_rate_bounds);
}

bool camera::get_frame_rate_enable() const
{
    return call_camera_fn(arv_camera_get_frame_rate_enable);
}

double camera::get_gain() const
{
    return call_camera_fn(arv_camera_get_gain);
}

camera::auto_mode camera::get_gain_auto() const
{
    return static_cast<auto_mode>(call_camera_fn(arv_camera_get_gain_auto));
}

camera::bounds_t<double> camera::get_gain_bounds() const
{
    return call_camera_fn_to_get_bounds<double>(arv_camera_get_gain_bounds);
}

camera::representation camera::get_gain_representation() const
{
    return static_cast<representation>(arv_camera_get_gain_representation(m_camera));
}

camera::bounds_t<int32_t> camera::get_height_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_height_bounds);
}

int32_t camera::get_height_increment() const
{
    return call_camera_fn(arv_camera_get_height_increment);
}

int64_t camera::get_integer(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_integer, feature_utf8.c_str());
}

camera::bounds_t<int64_t> camera::get_integer_bounds(const std::string &feature_utf8) const
{
    return call_camera_fn_to_get_bounds<int64_t>(arv_camera_get_integer_bounds, feature_utf8.c_str());
}

int64_t camera::get_integer_increment(const std::string &feature_utf8) const
{
    return call_camera_fn(arv_camera_get_integer_increment, feature_utf8.c_str());
}

std::string camera::get_pixel_format() const
{
    return call_camera_fn(arv_camera_get_pixel_format_as_string);
}

camera::region_t camera::get_region() const
{
    region_t region;
    call_camera_fn_with_no_return(arv_camera_get_region, &region.offset.x, &region.offset.y, &region.size.width, &region.size.height);
    return region;
}

camera::rect_size_t camera::get_sensor_size() const
{
    rect_size_t rect_size;
    call_camera_fn_with_no_return(arv_camera_get_sensor_size, &rect_size.width, &rect_size.height);
    return rect_size;
}

camera::bounds_t<int32_t> camera::get_width_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_width_bounds);
}

int32_t camera::get_width_increment() const
{
    return call_camera_fn(arv_camera_get_width_increment);
}

camera::bounds_t<int32_t> camera::get_x_offset_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_x_offset_bounds);
}

int32_t camera::get_x_offset_increment() const
{
    return call_camera_fn(arv_camera_get_x_offset_increment);
}

camera::bounds_t<int32_t> camera::get_y_offset_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_y_offset_bounds);
}

int32_t camera::get_y_offset_increment() const
{
    return call_camera_fn(arv_camera_get_y_offset_increment);
}

bool camera::is_exposure_auto_available() const
{
    return call_camera_fn(arv_camera_is_exposure_auto_available);
}

bool camera::is_exposure_time_available() const
{
    return call_camera_fn(arv_camera_is_exposure_time_available);
}

bool camera::is_frame_rate_available() const
{
    return call_camera_fn(arv_camera_is_frame_rate_available);
}

bool camera::is_gain_auto_available() const
{
    return call_camera_fn(arv_camera_is_gain_auto_available);
}

bool camera::is_gain_available() const
{
    return call_camera_fn(arv_camera_is_gain_available);
}

bool camera::is_region_offset_available() const
{
    return call_camera_fn(arv_camera_is_region_offset_available);
}

void camera::select_gain(const std::string &selector_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_select_gain, selector_utf8.c_str());
}

void camera::set_exposure_mode(camera::exposure_mode mode) const
{
    call_camera_fn_with_no_return(arv_camera_set_exposure_mode, static_cast<ArvExposureMode>(mode));
}

void camera::set_exposure_time(double time_us) const
{
    call_camera_fn_with_no_return(arv_camera_set_exposure_time, time_us);
}

void camera::set_exposure_time_auto(camera::auto_mode mode) const
{
    call_camera_fn_with_no_return(arv_camera_set_exposure_time_auto, static_cast<ArvAuto>(mode));
}

void camera::set_float(const std::string &feature_utf8, double value) const
{
    call_camera_fn_with_no_return(arv_camera_set_float, feature_utf8.c_str(), value);
}

void camera::set_frame_count(int64_t count) const
{
    call_camera_fn_with_no_return(arv_camera_set_frame_count, count);
}

void camera::set_frame_rate(double rate) const
{
    call_camera_fn_with_no_return(arv_camera_set_frame_rate, rate);
}

void camera::set_frame_rate_enable(bool enable) const
{
    call_camera_fn_with_no_return(arv_camera_set_frame_rate_enable, enable);
}

void camera::set_gain(double gain) const
{
    call_camera_fn_with_no_return(arv_camera_set_gain, gain);
}

void camera::set_gain_auto(auto_mode mode) const
{
    call_camera_fn_with_no_return(arv_camera_set_gain_auto, static_cast<ArvAuto>(mode));
}

void camera::set_integer(const std::string &feature_utf8, int64_t value) const
{
    call_camera_fn_with_no_return(arv_camera_set_integer, feature_utf8.c_str(), value);
}

void camera::set_region(const camera::region_t &region) const
{
    call_camera_fn_with_no_return(arv_camera_set_region, region.offset.x, region.offset.y, region.size.width, region.size.height);
}

void camera::set_register(const std::string &register_utf8, const std::vector<std::byte> &bytes) const
{
    call_camera_fn_with_no_return(arv_camera_set_register, register_utf8.c_str(), bytes.size(), const_cast<void *>(reinterpret_cast<const void *>(bytes.data())));
}

uint64_t camera::get_stream_frame_count() const
{
    return m_stream_frames_count;
}

uint64_t camera::get_stream_error_count() const
{
    return m_stream_frames_error_count;
}

camera::binning_t camera::get_binning() const
{
    binning_t b;
    call_camera_fn_with_no_return(arv_camera_get_binning, &b.dx, &b.dy);
    return b;
}

camera::bounds_t<int32_t> camera::get_x_binning_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_x_binning_bounds);
}

camera::bounds_t<int32_t> camera::get_y_binning_bounds() const
{
    return call_camera_fn_to_get_bounds<int32_t>(arv_camera_get_y_binning_bounds);
}

void camera::set_binning(const binning_t &binning) const
{
    call_camera_fn_with_no_return(arv_camera_set_binning, binning.dx, binning.dy);
}

bool camera::is_binning_available() const
{
    return call_camera_fn(arv_camera_is_binning_available);
}

int32_t camera::get_x_binning_increment() const
{
    return call_camera_fn(arv_camera_get_x_binning_increment);
}

int32_t camera::get_y_binning_increment() const
{
    return call_camera_fn(arv_camera_get_y_binning_increment);
}

std::string_view camera::get_genicam_xml() const
{

    size_t length;

    auto char_ptr = arv_device_get_genicam_xml(arv_camera_get_device(m_camera), &length);

    return std::string_view{char_ptr, length};
}

double camera::get_black_level() const
{
    return call_camera_fn(arv_camera_get_black_level);
}

camera::auto_mode camera::get_black_level_auto() const
{
    return static_cast<auto_mode>(call_camera_fn(arv_camera_get_black_level_auto));
}

camera::bounds_t<double> camera::get_black_level_bounds() const
{
    return call_camera_fn_to_get_bounds<double>(arv_camera_get_black_level_bounds);
}

bool camera::is_black_level_auto_available() const
{
    return call_camera_fn(arv_camera_is_black_level_auto_available);
}

void camera::set_black_level(double level) const
{
    call_camera_fn_with_no_return(arv_camera_set_black_level, level);
}

void camera::set_black_level_auto(camera::auto_mode mode) const
{
    call_camera_fn_with_no_return(arv_camera_set_black_level_auto, static_cast<ArvAuto>(mode));
}

void camera::select_black_level(const std::string &selector_utf8) const
{
    call_camera_fn_with_no_return(arv_camera_select_black_level, selector_utf8.c_str());
}

bool camera::is_black_level_available() const
{
    return call_camera_fn(arv_camera_is_black_level_available);
}

void camera::set_access_mode_policy(bool enable) const
{
    arv_camera_set_access_check_policy(m_camera,
                                       enable ? ARV_ACCESS_CHECK_POLICY_ENABLE : ARV_ACCESS_CHECK_POLICY_DISABLE);
}

void camera::set_range_mode_policy(bool enable) const
{
    arv_camera_set_range_check_policy(m_camera,
                                      enable ? ARV_RANGE_CHECK_POLICY_ENABLE : ARV_RANGE_CHECK_POLICY_DISABLE);
}

bool camera::select_component(const std::string &component_utf8, const component_selection_flag flag, uint32_t *component_id) const
{
    ArvComponentSelectionFlags f;
    switch (flag)
    {
    case component_selection_flag::none:
        f = ARV_COMPONENT_SELECTION_FLAGS_NONE;
        break;
    case component_selection_flag::enable:
        f = ARV_COMPONENT_SELECTION_FLAGS_ENABLE;
        break;
    case component_selection_flag::disable:
        f = ARV_COMPONENT_SELECTION_FLAGS_DISABLE;
        break;
    case component_selection_flag::enable_all:
        f = ARV_COMPONENT_SELECTION_FLAGS_ENABLE_ALL;
        break;
    case component_selection_flag::exclusive_enable:
        f = ARV_COMPONENT_SELECTION_FLAGS_EXCLUSIVE_ENABLE;
        break;
    };

    return call_camera_fn(arv_camera_select_component, component_utf8.c_str(), f, component_id);
}

void camera::select_and_enable_component(const std::string &component_utf8, bool disable_others) const
{
    call_camera_fn_with_no_return(arv_camera_select_and_enable_component, component_utf8.c_str(), disable_others);
}

int64_t camera::get_frame_count() const
{
    return call_camera_fn(arv_camera_get_frame_count);
}

camera::bounds_t<int64_t> camera::get_frame_count_bounds() const
{
    return call_camera_fn_to_get_bounds<int64_t>(arv_camera_get_frame_count_bounds);
}

void camera::set_gv_socket_buffer_size(int32_t size)
{
    if (!arv_camera_is_gv_device(m_camera))
    {
        throw std::invalid_argument("Unable to set this property for a non-GigE device.");
    }

    if (m_stream)
    {
        throw std::runtime_error("Unable to set the socket-buffer-size whilst the camera is streaming.");
    }

    m_gv_buffer_size = size;
}

bool camera::is_gv_device() const
{
    return arv_camera_is_gv_device(m_camera);
}

uint32_t camera::get_gv_auto_packet_size() const
{
    if (!arv_camera_is_gv_device(m_camera))
    {
        throw std::invalid_argument("Unable to set this property for a non-GigE device.");
    }

    return call_camera_fn(arv_camera_gv_auto_packet_size);
}

void camera::set_gv_packet_size(int32_t size)
{
    if (!arv_camera_is_gv_device(m_camera))
    {
        throw std::invalid_argument("Unable to set this property for a non-GigE device.");
    }

    call_camera_fn_with_no_return(arv_camera_gv_set_packet_size, size);
}