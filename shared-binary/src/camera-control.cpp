#include <stdexcept>
#include <system_error>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/device-io/camera.hpp"
#include "g_industrial_cam/lv_interop/lv_str_array_1d.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

namespace
{
    using camera_handle = EDVRManagedObject<camera>;
    using LV_UserEventRefPtr_t = LV_Ptr_t<LV_UserEventRef_t>;
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_create(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_StringHandle_t id_handle,
        LV_UserEventRefPtr_t disconnect_user_event_ref,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            LV_UserEventRef_t disconnect = *disconnect_user_event_ref;
            camera_handle(edvr_ref_ptr, camera::create(id_handle.to_utf8_string(), [=]
                                                       {
                LV_Boolean_t data = false;
                PostLVUserEvent(disconnect,&data); }));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_available_pixel_formats(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_1DStringArrayHandle_t formats_handle)
    {
        try
        {
            std::vector<std::string> formats;
            camera_handle(edvr_ref_ptr)->available_pixel_formats(formats);

            formats_handle.copy_from_utf8(formats);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_pixel_format(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t format_handle)
    {
        try
        {
            format_handle.copy_from_utf8(camera_handle(edvr_ref_ptr)->get_pixel_format());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_pixel_format(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t format_handle)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_pixel_format(format_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_take_snapshot(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t timeout_ms,
        LV_EDVRReferencePtr_t buffer_ref_ptr)
    {
        try
        {
            auto buf = camera_handle(camera_ref_ptr)->take_snapshot(timeout_ms);

            if (arv_buffer_get_status(buf) != ARV_BUFFER_STATUS_SUCCESS)
            {
                auto status = arv_buffer_get_status(buf);
                return LV_ERR_ncTimeOutErr;
            }

            lv_buffer(buffer_ref_ptr, buf);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_stream_start(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint16_t additional_buffers,
        int32_t max_sequential_errors,
        int32_t timeout_ms,
        LV_BooleanPtr_t fixed_frame_count,
        LV_UserEventRefPtr_t on_stream_error_event,
        LV_UserEventRefPtr_t on_stream_stop_event)
    {
        try
        {
            LV_UserEventRef_t error = *on_stream_error_event;
            LV_UserEventRef_t stop = *on_stream_stop_event;

            camera_handle(camera_ref_ptr)->stream_start(max_sequential_errors, additional_buffers, timeout_ms, *fixed_frame_count, [=]()
                                                        {
                LV_Boolean_t data = false;
                PostLVUserEvent(error,&data); }, [=]()
                                                        {
                LV_Boolean_t data = false;
                PostLVUserEvent(stop,&data); });
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_stream_stop(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr)
    {
        try
        {
            camera_handle(camera_ref_ptr)->stream_stop();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_stream_pop_buffer(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_EDVRReferencePtr_t buffer_ref_ptr,
        int32_t timeout_ms)
    {
        try
        {
            auto result = camera_handle(camera_ref_ptr)->stream_pop_buffer(timeout_ms, lv_buffer(buffer_ref_ptr));

            if (result == camera::timeout_result::timeout)
            {
                throw std::system_error(56, std::iostream_category(), "A Timeout occured whilst waiting for a stream buffer.");
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_clear_triggers(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr)
    {
        try
        {
            camera_handle(camera_ref_ptr)->clear_triggers();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_trigger_sources(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t sources_handle)
    {
        try
        {
            std::vector<std::string> sources_utf8;
            camera_handle(camera_ref_ptr)->available_trigger_sources(sources_utf8);

            sources_handle.copy_from_utf8(sources_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_trigger_source(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t source_handle)
    {
        try
        {
            source_handle.copy_from_utf8(camera_handle(camera_ref_ptr)->get_trigger_source());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_software_trigger_supported(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_supported)
    {
        try
        {
            *is_supported = camera_handle(camera_ref_ptr)->is_software_trigger_supported();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_boolean_value(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t name_handle,
        LV_BooleanPtr_t value,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                camera_handle(camera_ref_ptr)->set_boolean(name_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = camera_handle(camera_ref_ptr)->get_boolean(name_handle.to_utf8_string());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_string_value(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t name_handle,
        LV_StringHandle_t value_handle,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                camera_handle(camera_ref_ptr)->set_string(name_handle.to_utf8_string(), value_handle.to_utf8_string());
            }
            else
            {
                value_handle.copy_from_utf8(camera_handle(camera_ref_ptr)->get_string(name_handle.to_utf8_string()));
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_trigger_source(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t source_handle)
    {
        try
        {
            camera_handle(camera_ref_ptr)->set_trigger(source_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_software_trigger(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr)
    {
        try
        {
            camera_handle(camera_ref_ptr)->software_trigger();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_black_levels(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void available_black_levels(std::vector<std::string> & black_levels_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_components(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void available_components(std::vector<std::string> & components_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_enumerations(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void available_enumerations(std::vector<std::string> & enums_utf8, const std::string &feature_utf) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_gains(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void available_gains(std::vector<std::string> & gains_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_read_register(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void read_register(const std::string &register_utf8, std::vector<std::byte> &bytes) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_execute_command(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            void execute_command(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_boolean(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bool get_boolean(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            double get_exposure_time() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            auto_mode get_exposure_time_auto() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<double> get_exposure_time_bounds() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time_representation(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            representation get_exposure_time_representation() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_float(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            double get_float(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_float_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<double> get_float_bounds(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_float_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            double get_float_increment(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_frame_rate(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            double get_frame_rate() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_frame_rate_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<double> get_frame_rate_bounds() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_frame_rate_enable(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bool get_frame_rate_enable() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            double get_gain() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            auto_mode get_gain_auto() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<double> get_gain_bounds() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain_representation(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            representation get_gain_representation() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_height_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<int32_t> get_height_bounds() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_height_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            int32_t get_height_increment() const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_integer_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            bounds_t<int64_t> get_integer_bounds(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
    
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_integer_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            int64_t get_integer_increment(const std::string &feature_utf8) const
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_region(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t* x,
        int32_t* y,
        int32_t* width,
        int32_t* height
    )
    {
        try
        {
            auto r = camera_handle(edvr_ref_ptr)->get_region();
            *x = r.offset.x;
            *y = r.offset.y;
            *width = r.size.width;
            *height = r.size.height;

        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_sensor_size(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t* width,
        int32_t* height
    )
    {
        try
        {
            auto s =  camera_handle(edvr_ref_ptr)->get_sensor_size();
            *width = s.width;
            *height = s.height;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_width_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = camera_handle(edvr_ref_ptr)->get_width_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_width_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *width)
    {
        try
        {
            *width = camera_handle(edvr_ref_ptr)->get_width_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_x_offset_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = camera_handle(edvr_ref_ptr)->get_x_offset_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_x_offset_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *offset)
    {
        try
        {
            *offset = camera_handle(edvr_ref_ptr)->get_x_offset_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_y_offset_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = camera_handle(edvr_ref_ptr)->get_y_offset_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_y_offset_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *offset)
    {
        try
        {
            *offset = camera_handle(edvr_ref_ptr)->get_y_offset_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_enumeration_entry_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_StringHandle_t entry_handle,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_enumeration_entry_available(feature_handle.to_utf8_string(), entry_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_exposure_auto_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_exposure_auto_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_exposure_time_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_exposure_time_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_feature_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_feature_available(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_feature_implemented(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_BooleanPtr_t is_implemented)
    {
        try
        {
            *is_implemented = camera_handle(edvr_ref_ptr)->is_feature_implemented(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_frame_rate_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_frame_rate_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_gain_auto_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_gain_auto_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_gain_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_gain_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_region_offset_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = camera_handle(edvr_ref_ptr)->is_region_offset_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_select_gain(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t selector_handle)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->select_gain(selector_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_exposure_mode(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint8_t mode)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_exposure_mode(static_cast<camera::exposure_mode>(mode));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_exposure_time(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        double time_us)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_exposure_time(time_us);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_exposure_time_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint8_t mode)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_exposure_time_auto(static_cast<camera::auto_mode>(mode));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_float(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t feature_handle,
        double *value,
        LV_BooleanPtr_t set)
    {
        try
        {
            camera_handle camera(edvr_ref_ptr);

            if (*set)
            {
                camera->set_float(feature_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = camera->get_float(feature_handle.to_utf8_string());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_frame_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int64_t count)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_frame_count(count);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_frame_rate(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        double rate)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_frame_rate(rate);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_frame_rate_enable(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t enable)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_frame_rate_enable(*enable);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_gain(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        double gain)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_gain(gain);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_gain_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint8_t mode)
    {
        try
        {
            camera::auto_mode modes[] = {
                camera::auto_mode::off,
                camera::auto_mode::once,
                camera::auto_mode::continuous};

            camera_handle(edvr_ref_ptr)->set_gain_auto(modes[mode]);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_integer(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t feature_handle,
        int64_t *value,
        LV_BooleanPtr_t set)
    {
        try
        {
            camera_handle camera(edvr_ref_ptr);

            if (*set)
            {
                camera->set_integer(feature_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = camera->get_integer(feature_handle.to_utf8_string());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_region(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t x,
        int32_t y,
        int32_t width,
        int32_t height)
    {
        try
        {
            camera_handle(edvr_ref_ptr)->set_region(camera::region_t{x, y, width, height});
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_register(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t register_handle,
        LV_StringHandle_t bytes_handle)
    {
        try
        {
            std::vector<std::byte> bytes;
            bytes.resize(bytes_handle.size());

            std::memcpy(bytes.data(), bytes_handle.begin(), bytes_handle.size());

            camera_handle(edvr_ref_ptr)->set_register(register_handle.to_utf8_string(), bytes);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_stream_frame_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint64_t *count)
    {
        try
        {
            *count = camera_handle(edvr_ref_ptr)->get_stream_frame_count();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_stream_error_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint64_t *count)
    {
        try
        {

            *count = camera_handle(edvr_ref_ptr)->get_stream_error_count();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}
