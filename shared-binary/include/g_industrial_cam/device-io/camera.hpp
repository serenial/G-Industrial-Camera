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
        enum class timeout_result : uint8_t
        {
            success,
            timeout
        };

        enum class auto_mode : uint8_t {
            off,
            once,
            continuous
        };

        enum class component_selection_flag : uint8_t {
            none,
            enable,
            disable,
            exclusive_enable,
            enable_all
        };

        template<class T>
        struct bounds_t{
            T min, max;
        };

        enum class representation : int32_t{
            undefined = -1,
            linear = 0,
            logarithmic,
            boolean,
            pure_number,
            hex_number,
            ip_address,
            mac_address
        };

        struct rect_size_t{
            int32_t width, height;
        };

        struct offset_t{
            int32_t x,y;
        };

        struct region_t{
            offset_t offset;
            rect_size_t size;
        };

        enum class exposure_mode : uint8_t{
            off,
            timed,
            trigger_width,
            trigger_controller
        };

        struct binning_t{
            int32_t dx, dy;
        };

        // two stage initializer as LabVIEW aborts on a failed constructor
        static camera *create(const std::string &identifier_utf8, signal_fn_t on_disconnect);
        ~camera();
        ArvBuffer *take_snapshot(int32_t timeout) const;
        timeout_result stream_start(int32_t max_sequential_errors, uint16_t circular_buffer_size, int32_t timeout_ms, bool fixed_num_frames, signal_fn_t on_stream_error, signal_fn_t on_stream_stop);
        void stream_stop();
        timeout_result stream_pop_buffer(int32_t timeout_ms, ArvBuffer **buffer_ptr);
        timeout_result stream_pop_buffer_back(int32_t timeout_ms, ArvBuffer **buffer_ptr);
        uint64_t stream_get_error_count() const;
        uint64_t stream_get_frame_count() const;

        void available_black_levels(std::vector<std::string> &black_levels_utf8) const;
        void available_components(std::vector<std::string> &components_utf8) const;
        void available_enumerations(const std::string &feature_utf, std::vector<std::string> &enums_utf8) const;
        void available_gains(std::vector<std::string> &gains_utf8) const;
        void available_trigger_sources(std::vector<std::string> &trigger_sources_utf8) const;
        void available_pixel_formats(std::vector<std::string> &pixel_formats_utf8) const;
        void available_triggers(std::vector<std::string> &triggers_utf8) const;
        void clear_triggers();
        void read_register(const std::string &register_utf8, std::vector<std::byte> &bytes) const;
        void execute_command(const std::string &command_utf8) const;
        bool get_boolean(const std::string &feature_utf8) const;
        double get_black_level() const;
        auto_mode get_black_level_auto() const;
        bounds_t<double> get_black_level_bounds() const;
        double get_exposure_time() const;
        auto_mode get_exposure_time_auto() const;
        bounds_t<double> get_exposure_time_bounds() const;
        representation get_exposure_time_representation() const;
        double get_float(const std::string& feature_utf8) const;
        bounds_t<double> get_float_bounds(const std::string& feature_utf8) const;
        representation get_feature_representation(const std::string& feature_utf8) const;
        double get_float_increment(const std::string& feature_utf8) const;
        double get_frame_rate() const;
        bounds_t<double> get_frame_rate_bounds() const;
        bool get_frame_rate_enable() const;
        int64_t get_frame_count() const;
        bounds_t<int64_t> get_frame_count_bounds() const;
        double get_gain() const;
        auto_mode get_gain_auto() const;
        bounds_t<double> get_gain_bounds() const;
        representation get_gain_representation() const;
        bounds_t<int32_t> get_height_bounds() const;
        int32_t get_height_increment() const;
        int64_t get_integer(const std::string& feature_utf8) const;
        bounds_t<int64_t> get_integer_bounds(const std::string& feature_utf8) const;
        int64_t get_integer_increment(const std::string& feature_utf8) const;
        std::string get_pixel_format() const;
        region_t get_region() const;
        rect_size_t get_sensor_size() const;
        std::string get_string(const std::string& feature_utf8) const;
        std::string get_trigger_source() const;
        bounds_t<int32_t> get_width_bounds() const;
        int32_t get_width_increment() const;
        bounds_t<int32_t> get_x_offset_bounds() const;
        int32_t get_x_offset_increment() const;
        bounds_t<int32_t> get_y_offset_bounds() const;
        int32_t get_y_offset_increment() const;
        binning_t get_binning() const;
        bounds_t<int32_t> get_x_binning_bounds() const;
        bounds_t<int32_t> get_y_binning_bounds() const;
        int32_t get_x_binning_increment() const;
        int32_t get_y_binning_increment() const;
        std::string_view get_genicam_xml() const;
        bool is_black_level_auto_available() const;
        bool is_black_level_available() const;
        bool is_component_available() const;
        bool is_enumeration_entry_available(const std::string &feature_utf8, const std::string &entry_utf8) const;
        bool is_exposure_auto_available() const;
        bool is_exposure_time_available() const;
        bool is_binning_available() const;
        bool is_feature_available(const std::string &feature_utf8) const;
        bool is_feature_implemented(const std::string &feature_utf8) const;
        bool is_frame_rate_available() const;
        bool is_gain_auto_available() const;
        bool is_gain_available() const;
        bool is_region_offset_available() const;
        void select_gain(const std::string& selector_utf8) const;
        void select_black_level(const std::string& selector_utf8) const;
        bool select_component(const std::string& component_utf8, const component_selection_flag flag, uint32_t* component_id) const;
        void select_and_enable_component(const std::string& component_utf8, bool disable_others) const;
        bool is_software_trigger_supported() const;
        void set_access_mode_policy(bool enable) const;
        void set_range_mode_policy(bool enable) const;
        void set_binning(const binning_t& binning) const;
        void set_boolean(const std::string &feature_utf8, bool value) const;
        void set_black_level(double gain) const;
        void set_black_level_auto(auto_mode mode) const;
        void set_exposure_mode(exposure_mode mode) const;
        void set_exposure_time(double time_us) const;
        void set_exposure_time_auto(auto_mode mode) const;
        void set_float(const std::string &feature_utf8, double value) const;
        void set_frame_count(int64_t count) const;
        void set_frame_rate(double rate) const;
        void set_frame_rate_enable(bool enable) const;
        void set_gain(double gain) const;
        void set_gain_auto(auto_mode mode) const;
        void set_integer(const std::string &feature_utf8, int64_t value) const;
        void set_pixel_format(const std::string &pixel_format_utf8);
        void set_region(const region_t& region) const;
        void set_register(const std::string &register_utf8, const std::vector<std::byte> &bytes) const;
        void set_string(const std::string &feature_utf8, const std::string &value_utf8) const;
        void set_trigger(const std::string &source_utf8) const;
        void set_trigger_source(const std::string &source_utf8) const;
        void software_trigger() const;
        uint64_t get_stream_frame_count() const;
        uint64_t get_stream_error_count() const;
        void set_gv_socket_buffer_size(int32_t size);
        bool is_gv_device() const;
        uint32_t get_gv_auto_packet_size() const;
        void set_gv_packet_size(int32_t size);

    private:
        camera(signal_fn_t on_disconnect);
        void connect(const std::string &identifier_utf8);
        static void stream_event_callback(void *self, ArvStreamCallbackType type, ArvBuffer *buffer_ptr);
        static void stream_buffer_callback(ArvStream *stream, camera *self);
        static void gv_control_lost_callback(ArvGvDevice *gv_device, camera *self);
        void populate_list_with_fn(std::vector<std::string> &list, std::add_pointer_t<const char **(ArvCamera *, guint *, GError **)> fn) const;
        gint m_gv_buffer_size;
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
        uint64_t m_stream_frames_error_count;
        uint64_t m_stream_frames_count;

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
        auto call_camera_fn(fn_t fn, args_t... args) const{
            aravis_error err;

            auto result = fn(m_camera, args..., err);

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

        template <typename bnds_t, typename fn_t, typename ...args_t>
        bounds_t<bnds_t> call_camera_fn_to_get_bounds(fn_t fn, args_t... args) const{
            bounds_t<bnds_t> b;
            call_camera_fn_with_no_return(fn, args..., &b.min, &b.max);
            return b;
        }
        
    };
}