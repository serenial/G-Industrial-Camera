#include <stdexcept>
#include <system_error>
#include <mutex>

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
    class shared_camera
    {
        std::shared_ptr<camera> m_ptr;

    public:
        camera *operator->()
        {
            return m_ptr.get();
        }
        camera *operator->() const
        {
            return m_ptr.get();
        }

        shared_camera(std::shared_ptr<camera> p) : m_ptr(p) {}
    };

    // create a shared_camera class that allows us to share a single camera instance
    using camera_handle = EDVRManagedObject<shared_camera>;
    using LV_UserEventRefPtr_t = LV_Ptr_t<LV_UserEventRef_t>;
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_create(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_StringHandle_t id_handle,
        LV_UserEventRefPtr_t disconnect_user_event_ref,
        LV_EDVRReferencePtr_t primary_camera_ref_ptr,
        LV_EDVRReferencePtr_t secondary_camera_ref_ptr)
    {
        try
        {
            LV_UserEventRef_t disconnect = *disconnect_user_event_ref;
            auto shared = std::shared_ptr<camera>(camera::create(id_handle.to_utf8_string(), [=]
                                                                 {
                LV_Boolean_t data = false;
                PostLVUserEvent(disconnect,&data); }));

            camera_handle primary(primary_camera_ref_ptr, new shared_camera(shared));
            camera_handle secondary(secondary_camera_ref_ptr, new shared_camera(shared));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_pixel_formats(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t formats_handle)
    {
        try
        {
            std::vector<std::string> formats;

            (*camera_handle(camera_ref_ptr))->available_pixel_formats(formats);

            formats_handle.copy_from_utf8(formats);
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
            auto buf = (*camera_handle(camera_ref_ptr))->take_snapshot(timeout_ms);

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

            (*camera_handle(camera_ref_ptr))->stream_start(max_sequential_errors, additional_buffers, timeout_ms, *fixed_frame_count, [=]()
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
            (*camera_handle(camera_ref_ptr))->stream_stop();
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
            auto result = (*camera_handle(camera_ref_ptr))->stream_pop_buffer(timeout_ms, lv_buffer(buffer_ref_ptr));

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
            (*camera_handle(camera_ref_ptr))->clear_triggers();
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
            (*camera_handle(camera_ref_ptr))->available_trigger_sources(sources_utf8);

            sources_handle.copy_from_utf8(sources_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_triggers(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t triggers_handle)
    {
        try
        {
            std::vector<std::string> triggers_utf8;
            (*camera_handle(camera_ref_ptr))->available_triggers(triggers_utf8);

            triggers_handle.copy_from_utf8(triggers_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_trigger_source(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t source_handle,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_trigger_source(source_handle.to_utf8_string());
            }
            else
            {
                source_handle.copy_from_utf8((*camera_handle(camera_ref_ptr))->get_trigger_source());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_configure_trigger(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t source_handle)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->set_trigger(source_handle.to_utf8_string());
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
            *is_supported = (*camera_handle(camera_ref_ptr))->is_software_trigger_supported();
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
                (*camera_handle(camera_ref_ptr))->set_boolean(name_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = (*camera_handle(camera_ref_ptr))->get_boolean(name_handle.to_utf8_string());
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
                (*camera_handle(camera_ref_ptr))->set_string(name_handle.to_utf8_string(), value_handle.to_utf8_string());
            }
            else
            {
                value_handle.copy_from_utf8((*camera_handle(camera_ref_ptr))->get_string(name_handle.to_utf8_string()));
            }
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
            (*camera_handle(camera_ref_ptr))->software_trigger();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_black_levels(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t levels_handle)
    {
        try
        {
            std::vector<std::string> levels_utf8;
            (*camera_handle(camera_ref_ptr))->available_black_levels(levels_utf8);

            levels_handle.copy_from_utf8(levels_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_components(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t components)
    {
        try
        {
            std::vector<std::string> components_utf8;

            (*camera_handle(camera_ref_ptr))->available_components(components_utf8);

            components.copy_from_utf8(components_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_enumerations(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_1DStringArrayHandle_t enum_handle)
    {
        try
        {
            std::vector<std::string> enums_utf8;
            (*camera_handle(camera_ref_ptr))->available_enumerations(feature_handle.to_utf8_string(), enums_utf8);
            enum_handle.copy_from_utf8(enums_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_gains(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_1DStringArrayHandle_t gains_handle)
    {
        try
        {
            std::vector<std::string> gains_utf8;
            (*camera_handle(camera_ref_ptr))->available_gains(gains_utf8);
            gains_handle.copy_from_utf8(gains_utf8);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_execute_command(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t command_handle)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->execute_command(command_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_xml(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t xml_handle)
    {
        try
        {
            xml_handle.copy_from_utf8(std::string{(*camera_handle(camera_ref_ptr))->get_genicam_xml()});
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *min,
        double *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_exposure_time_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_exposure_time_representation(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t *representation)
    {
        try
        {
            *representation = static_cast<uint8_t>(static_cast<int32_t>((*camera_handle(camera_ref_ptr))->get_exposure_time_representation()) + 1);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_float_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        double *min,
        double *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_float_bounds(feature_handle.to_utf8_string());
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_feature_representation(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        uint8_t *representation)
    {
        try
        {
            *representation = static_cast<uint8_t>(static_cast<int32_t>((*camera_handle(camera_ref_ptr))->get_feature_representation(feature_handle.to_utf8_string())) + 1);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_float_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        double *increment)
    {
        try
        {
            *increment = (*camera_handle(camera_ref_ptr))->get_float_increment(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_frame_rate_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *min,
        double *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_frame_rate_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *min,
        double *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_gain_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_gain_representation(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t *representation)
    {
        try
        {
            *representation = static_cast<uint8_t>(static_cast<int32_t>((*camera_handle(camera_ref_ptr))->get_gain_representation()) + 1);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_height_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_height_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_height_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *increment)
    {
        try
        {
            *increment = (*camera_handle(camera_ref_ptr))->get_height_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_integer_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        int64_t *min,
        int64_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_integer_bounds(feature_handle.to_utf8_string());
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_integer_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        int64_t *increment)
    {
        try
        {
            *increment = (*camera_handle(camera_ref_ptr))->get_integer_increment(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_region(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *x,
        int32_t *y,
        int32_t *width,
        int32_t *height,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_region(camera::region_t{*x, *y, *width, *height});
            }
            else
            {
                auto r = (*camera_handle(camera_ref_ptr))->get_region();
                *x = r.offset.x;
                *y = r.offset.y;
                *width = r.size.width;
                *height = r.size.height;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_sensor_size(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *width,
        int32_t *height)
    {
        try
        {
            auto s = (*camera_handle(camera_ref_ptr))->get_sensor_size();
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
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_width_bounds();
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
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *width)
    {
        try
        {
            *width = (*camera_handle(camera_ref_ptr))->get_width_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_x_offset_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_x_offset_bounds();
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
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *offset)
    {
        try
        {
            *offset = (*camera_handle(camera_ref_ptr))->get_x_offset_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_y_offset_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_y_offset_bounds();
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
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *offset)
    {
        try
        {
            *offset = (*camera_handle(camera_ref_ptr))->get_y_offset_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_enumeration_entry_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_StringHandle_t entry_handle,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_enumeration_entry_available(feature_handle.to_utf8_string(), entry_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_exposure_auto_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_exposure_auto_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_exposure_time_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_exposure_time_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_feature_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_feature_available(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_binning_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_binning_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_feature_implemented(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        LV_BooleanPtr_t is_implemented)
    {
        try
        {
            *is_implemented = (*camera_handle(camera_ref_ptr))->is_feature_implemented(feature_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_frame_rate_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_frame_rate_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_gain_auto_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_gain_auto_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_gain_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_gain_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_region_offset_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_region_offset_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_select_gain(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t selector_handle)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->select_gain(selector_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_exposure_mode(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t mode)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->set_exposure_mode(static_cast<camera::exposure_mode>(mode));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_exposure_time(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *time_us,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_exposure_time(*time_us);
            }
            else
            {
                *time_us = (*camera_handle(camera_ref_ptr))->get_exposure_time();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_exposure_time_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t *mode,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_exposure_time_auto(static_cast<camera::auto_mode>(*mode));
            }
            else
            {
                *mode = static_cast<uint8_t>((*camera_handle(camera_ref_ptr))->get_exposure_time_auto());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_float(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        double *value,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_float(feature_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = (*camera_handle(camera_ref_ptr))->get_float(feature_handle.to_utf8_string());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_frame_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int64_t *count,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_frame_count(*count);
            }
            else
            {
                *count = (*camera_handle(camera_ref_ptr))->get_frame_count();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_frame_rate(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *rate,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_frame_rate(*rate);
            }
            else
            {
                *rate = (*camera_handle(camera_ref_ptr))->get_frame_rate();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_frame_rate_enable(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t enable,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_frame_rate_enable(*enable);
            }
            else
            {
                *enable = (*camera_handle(camera_ref_ptr))->get_frame_rate_enable();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_gain(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *gain,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_gain(*gain);
            }
            else
            {
                *gain = (*camera_handle(camera_ref_ptr))->get_gain();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_gain_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t *mode,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_gain_auto(static_cast<camera::auto_mode>(*mode));
            }
            else
            {
                *mode = static_cast<uint8_t>((*camera_handle(camera_ref_ptr))->get_gain_auto());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_integer(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t feature_handle,
        int64_t *value,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_integer(feature_handle.to_utf8_string(), *value);
            }
            else
            {
                *value = (*camera_handle(camera_ref_ptr))->get_integer(feature_handle.to_utf8_string());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_pixel_format(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t format_handle,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_pixel_format(format_handle.to_utf8_string());
            }
            {
                format_handle.copy_from_utf8((*camera_handle(camera_ref_ptr))->get_pixel_format());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_register(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t register_handle,
        LV_StringHandle_t bytes_handle,
        LV_BooleanPtr_t set)
    {
        try
        {
            std::vector<std::byte> bytes;

            if (*set)
            {
                bytes.resize(bytes_handle.size());

                std::memcpy(bytes.data(), bytes_handle.begin(), bytes_handle.size());

                (*camera_handle(camera_ref_ptr))->set_register(register_handle.to_utf8_string(), bytes);
            }
            else
            {
                (*camera_handle(camera_ref_ptr))->read_register(register_handle.to_utf8_string(), bytes);

                bytes_handle.size_to_fit(bytes.size());

                std::memcpy(bytes_handle.begin(), bytes.data(), bytes.size());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_stream_frame_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint64_t *count)
    {
        try
        {
            *count = (*camera_handle(camera_ref_ptr))->get_stream_frame_count();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_stream_error_count(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint64_t *count)
    {
        try
        {

            *count = (*camera_handle(camera_ref_ptr))->get_stream_error_count();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_binning(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *dx,
        int32_t *dy,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_binning(camera::binning_t{*dx, *dy});
            }
            else
            {
                auto b = (*camera_handle(camera_ref_ptr))->get_binning();
                *dx = b.dx;
                *dy = b.dy;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_x_binning_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_x_binning_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_y_binning_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *min,
        int32_t *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_y_binning_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_x_binning_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *increment)
    {
        try
        {
            *increment = (*camera_handle(camera_ref_ptr))->get_x_binning_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_y_binning_increment(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        int32_t *increment)
    {
        try
        {
            *increment = (*camera_handle(camera_ref_ptr))->get_y_binning_increment();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_black_level(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *black_level,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_black_level(*black_level);
            }
            else
            {
                *black_level = (*camera_handle(camera_ref_ptr))->get_black_level();
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_select_black_level(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t selector_handle)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->select_black_level(selector_handle.to_utf8_string());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_black_level_auto_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_black_level_auto_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_black_level_bounds(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        double *min,
        double *max)
    {
        try
        {
            auto b = (*camera_handle(camera_ref_ptr))->get_black_level_bounds();
            *min = b.min;
            *max = b.max;
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_black_level_auto(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint8_t *mode,
        LV_BooleanPtr_t set)
    {
        try
        {
            if (*set)
            {
                (*camera_handle(camera_ref_ptr))->set_black_level_auto(static_cast<camera::auto_mode>(*mode));
            }
            else
            {
                *mode = static_cast<uint8_t>((*camera_handle(camera_ref_ptr))->get_black_level_auto());
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_is_black_level_available(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t is_available)
    {
        try
        {
            *is_available = (*camera_handle(camera_ref_ptr))->is_black_level_available();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_access_mode_policy(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t enable)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->set_access_mode_policy(*enable);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_set_range_mode_policy(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_BooleanPtr_t enable)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->set_range_mode_policy(*enable);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_select_and_enable_component(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t component,
        LV_BooleanPtr_t disable_others)
    {
        try
        {
            (*camera_handle(camera_ref_ptr))->select_and_enable_component(component.to_utf8_string(), *disable_others);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_select_component(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t component,
        uint8_t flag,
        LV_BooleanPtr_t enabled,
        uint32_t *component_id)
    {
        try
        {
            *enabled = (*camera_handle(camera_ref_ptr))->select_component(component.to_utf8_string(), static_cast<camera::component_selection_flag>(flag), component_id);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}
