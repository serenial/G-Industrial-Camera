#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <boost/circular_buffer.hpp>

#include <arv.h>

namespace g_industrial_cam{
    class camera{
        public:
        using signal_fn_t = std::function<void()>;
         // two stage intializer as LabVIEW aborts on a failed constructor
        static camera* create(const std::string& identifier_utf8, signal_fn_t on_disconnect);
        ~camera();
        ArvBuffer* take_snapshot(int32_t timeout) const;
        void get_avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        void stream_start(uint16_t n_additional_buffers, signal_fn_t on_stream_start, signal_fn_t on_stream_stop);
        void stream_stop();
        void stream_pop_buffer(int32_t timeout_ms, ArvBuffer** buffer_ptr, 
            signal_fn_t on_stream_capture_success, signal_fn_t on_stream_capture_error, signal_fn_t on_stream_capture_timeout);
        private:
        camera(signal_fn_t on_disconnect);
        void connect(const std::string& identifier_utf8);
        static void stream_callback(void* self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        static void control_lost(ArvGvDevice *gv_device, void* self);
        ArvCamera *m_camera;
        ArvStream *m_stream;
        size_t m_camera_payload;
        boost::circular_buffer<ArvBuffer*> m_stream_buffers;
        std::mutex m_stream_buffers_mtx;
        std::condition_variable m_stream_event;
        const signal_fn_t m_callback_on_disconnect;
        signal_fn_t m_callback_on_stream_start;
        signal_fn_t m_callback_on_stream_stop;
        signal_fn_t m_callback_on_stream_capture_ok;
        signal_fn_t m_callback_on_stream_capture_error;
        signal_fn_t m_callback_on_stream_capture_timeout;
    };
}