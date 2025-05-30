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
        enum class timeout_result {
            success,
            timeout
        };
         // two stage intializer as LabVIEW aborts on a failed constructor
        static camera* create(const std::string& identifier_utf8, signal_fn_t on_disconnect);
        ~camera();
        ArvBuffer* take_snapshot(int32_t timeout) const;
        void get_avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        timeout_result stream_start(int32_t max_sequential_errors, uint16_t n_additional_buffers,int32_t timeout_ms, signal_fn_t on_stream_error, signal_fn_t on_stream_stop);
        void stream_stop();
        timeout_result stream_pop_buffer(int32_t timeout_ms, ArvBuffer** buffer_ptr);
        private:
        camera(signal_fn_t on_disconnect);
        void connect(const std::string& identifier_utf8);
        static void stream_event_callback(void* self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        static void stream_buffer_callback(ArvStream* stream, camera* self);
        static void gv_control_lost_callback(ArvGvDevice *gv_device, camera* self);
        ArvCamera *m_camera;
        ArvStream *m_stream;
        size_t m_camera_payload;
        boost::circular_buffer<ArvBuffer*> m_stream_buffers;
        std::mutex m_stream_buffers_mtx;
        std::condition_variable m_stream_event;
        bool m_stream_started;
        const signal_fn_t m_callback_on_disconnect;
        signal_fn_t m_callback_on_stream_stop;
        signal_fn_t m_callback_on_stream_error_limit_exceeded;
        int32_t m_stream_max_sequential_errors;
        int32_t m_stream_sequential_error_count;
    };
}