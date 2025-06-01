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
        std::string get_pixel_format() const;
        void set_pixel_format(const std::string& pixel_format_utf8);
        timeout_result stream_start(int32_t max_sequential_errors, uint16_t n_additional_buffers,int32_t timeout_ms, signal_fn_t on_stream_error, signal_fn_t on_stream_stop);
        void stream_stop();
        timeout_result stream_pop_buffer(int32_t timeout_ms, ArvBuffer** buffer_ptr);
        void clear_triggers();
        void available_black_levels(std::vector<std::string>& black_levels_utf8) const;
        void available_components(std::vector<std::string>& components_utf8) const;
        void available_enumerations(std::vector<std::string>& enums_utf8, std::string feature_utf) const;
        void available_gains(std::vector<std::string>& gains_utf8) const;
        void available_trigger_sources(std::vector<std::string>& trigger_sources_utf8) const;
        void avaliable_pixel_formats(std::vector<std::string>& pixel_formats_utf8) const;
        void available_triggers(std::vector<std::string>& triggers_utf8) const;

// dup_available_triggers
// dup_register
// execute_command
// get_acquisition_mode
// get_binning
// get_black_level
// get_black_level_auto
// get_black_level_bounds
// get_boolean
// get_boolean_gi
// get_exposure_time
// get_exposure_time_auto
// get_exposure_time_bounds
// get_exposure_time_representation
// get_feature_representation
// get_float
// get_float_bounds
// get_float_increment
// get_frame_count
// get_frame_count_bounds
// get_frame_rate
// get_frame_rate_bounds
// get_frame_rate_enable
// get_gain
// get_gain_auto
// get_gain_bounds
// get_gain_representation
// get_height_bounds
// get_height_increment
// get_integer
// get_integer_bounds
// get_integer_increment
// get_model_name
// get_region
// get_sensor_size
// get_string
// get_trigger_source
// get_width_bounds
// get_width_increment
// get_x_binning_bounds
// get_x_binning_increment
// get_x_offset_bounds
// get_x_offset_increment
// get_y_binning_bounds
// get_y_binning_increment
// get_y_offset_bounds
// get_y_offset_increment
// is_binning_available
// is_black_level_auto_available
// is_black_level_available
// is_component_available
// is_enumeration_entry_available
// is_exposure_auto_available
// is_exposure_time_available
// is_feature_available
// is_feature_implemented
// is_frame_rate_available
// is_gain_auto_available
// is_gain_available
// is_region_offset_available
// is_software_trigger_supported
// select_and_enable_component
// select_black_level
// select_component
// select_gain
// set_access_check_policy
// set_acquisition_mode
// set_binning
// set_black_level
// set_black_level_auto
// set_boolean
// set_exposure_mode
// set_exposure_time
// set_exposure_time_auto
// set_float
// set_frame_count
// set_frame_rate
// set_frame_rate_enable
// set_gain
// set_gain_auto
// set_integer
// set_range_check_policy
// set_region
// set_register
// set_register_cache_policy
// set_string
// set_trigger
// set_trigger_source
// software_trigger

        private:
        camera(signal_fn_t on_disconnect);
        void connect(const std::string& identifier_utf8);
        static void stream_event_callback(void* self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        static void stream_buffer_callback(ArvStream* stream, camera* self);
        static void gv_control_lost_callback(ArvGvDevice *gv_device, camera* self);
        void populate_list_with_fn(std::vector<std::string>&list,std::function<const char**(ArvCamera*, guint*, GError**)> fn) const;
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