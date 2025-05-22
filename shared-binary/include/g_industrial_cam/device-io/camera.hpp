#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <boost/circular_buffer.hpp>

#include <arv.h>

namespace g_industrial_cam{
    class camera{
        public:
         // two stage intializer as LabVIEW aborts on a failed constructor
        static camera* create(const std::string& identifier_utf8);
        ~camera();
        ArvBuffer* take_snapshot(int32_t timeout) const;
        void get_avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        void stream_start(uint16_t n_additional_buffers);
        void stream_stop();
        bool stream_pop_buffer(int32_t timeout_ms, ArvBuffer** buffer_ptr);
        private:
        camera() = default;
        void connect(const std::string& identifier_utf8);
        static void stream_callback(void* self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        ArvCamera *m_camera;
        ArvStream *m_stream;
        size_t m_camera_payload;
        boost::circular_buffer<ArvBuffer*> m_stream_buffers;
        std::mutex m_stream_buffers_mtx;
        std::condition_variable m_stream_event;
    };
}