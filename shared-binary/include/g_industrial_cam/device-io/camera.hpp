#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <boost/circular_buffer.hpp>

#include <arv.h>

#include "./aravis-error.hpp"

namespace g_industrial_cam
{

    class camera
    {
    public:
        using signal_fn_t = std::function<void()>;
        enum class timeout_result
        {
            success,
            timeout
        };
        // two stage intializer as LabVIEW aborts on a failed constructor
        static camera *create(const std::string &identifier_utf8, signal_fn_t on_disconnect);
        ~camera();
        ArvBuffer *take_snapshot(int32_t timeout) const;

        std::string get_pixel_format() const;
        void set_pixel_format(const std::string &pixel_format_utf8);

        timeout_result stream_start(int32_t max_sequential_errors, uint16_t n_additional_buffers, int32_t timeout_ms, signal_fn_t on_stream_error, signal_fn_t on_stream_stop);
        void stream_stop();
        timeout_result stream_pop_buffer(int32_t timeout_ms, ArvBuffer **buffer_ptr);
        void clear_triggers();
        void available_black_levels(std::vector<std::string> &black_levels_utf8) const;
        void available_components(std::vector<std::string> &components_utf8) const;
        void available_enumerations(std::vector<std::string> &enums_utf8, const std::string &feature_utf) const;
        void available_gains(std::vector<std::string> &gains_utf8) const;
        void available_trigger_sources(std::vector<std::string> &trigger_sources_utf8) const;
        void avaliable_pixel_formats(std::vector<std::string> &pixel_formats_utf8) const;
        void available_triggers(std::vector<std::string> &triggers_utf8) const;
        void read_register(std::vector<std::byte> &bytes, const std::string &register_utf8) const;
        void execute_command(const std::string &feature_utf8) const;

        // get_acquisition_mode
        // get_binning
        // get_black_level
        // get_black_level_auto
        // get_black_level_bounds

        bool get_boolean(const std::string &feature_utf8) const;

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
        std::string get_string(const std::string& feature_utf8) const;

        std::string get_trigger_source() const;

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

        bool is_enumeration_entry_available(const std::string &feature_utf8, const std::string &entry_utf8) const;

        // is_exposure_auto_available
        // is_exposure_time_available

        bool is_feature_available(const std::string &feature_utf8) const;
        bool is_feature_implemented(const std::string &feature_utf8) const;

        // is_frame_rate_available
        // is_gain_auto_available
        // is_gain_available
        // is_region_offset_available

        bool is_software_trigger_supported() const;

        // select_and_enable_component
        // select_black_level
        // select_component
        // select_gain
        // set_access_check_policy
        // set_acquisition_mode
        // set_binning
        // set_black_level
        // set_black_level_auto

        void set_boolean(const std::string &feature_utf8, bool value) const;

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
        void set_string(const std::string &feature_utf8, const std::string &value_utf8) const;
        void set_trigger(const std::string &source_utf8) const;
        void set_trigger_source(const std::string &source_utf8) const;
        void software_trigger() const;

    private:
        camera(signal_fn_t on_disconnect);
        void connect(const std::string &identifier_utf8);
        static void stream_event_callback(void *self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        static void stream_buffer_callback(ArvStream *stream, camera *self);
        static void gv_control_lost_callback(ArvGvDevice *gv_device, camera *self);
        void populate_list_with_fn(std::vector<std::string> &list, std::add_pointer_t<const char **(ArvCamera *, guint *, GError **)> fn) const;
        ArvCamera *m_camera;
        ArvStream *m_stream;
        size_t m_camera_payload;
        boost::circular_buffer<ArvBuffer *> m_stream_buffers;
        std::mutex m_stream_buffers_mtx;
        std::condition_variable m_stream_event;
        bool m_stream_started;
        const signal_fn_t m_callback_on_disconnect;
        signal_fn_t m_callback_on_stream_stop;
        signal_fn_t m_callback_on_stream_error_limit_exceeded;
        int32_t m_stream_max_sequential_errors;
        int32_t m_stream_sequential_error_count;

        template <class T>
        struct deleter_for_g_pointer
        {
            void operator()(T ptr)
            {
                g_free(ptr);
            }
        };

        template <typename fn_t, typename... args_t>
        void call_camera_fn_with_no_return(fn_t fn, args_t... args) const{
            aravis_error err;

            fn(m_camera, args..., err);

            aravis_error::check(err);
        }

        template <typename fn_t, typename... args_t>
        bool call_camera_fn_with_bool_return(fn_t fn, args_t... args) const{
            aravis_error err;

            gboolean result = fn(m_camera, args..., err);

            aravis_error::check(err);

            return result;
        }

        template <typename fn_t, typename... args_t>
        std::string call_camera_fn_with_string_return(fn_t fn, args_t... args) const{
            aravis_error err;

            std::string result(fn(m_camera, args..., err));

            aravis_error::check(err);

            return result;
        }

        template <typename fn_t, typename... args_t>
        void call_camera_fn_to_populate_list(fn_t fn, std::vector<std::string> &list, args_t... args) const{
            
            guint n_elements = 0;
            aravis_error err;

            std::unique_ptr<const char *, deleter_for_g_pointer<const char **>> result(fn(m_camera, args..., &n_elements, err));

            aravis_error::check(err);

            for (guint i = 0; i < n_elements; i++)
            {
                list.emplace_back(result.get()[i]);
            }
        }
        
    };
}