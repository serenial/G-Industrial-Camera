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

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_available_pixel_formats(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_1DStringArrayHandle_t formats_handle
    )
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

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_trigger(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        LV_StringHandle_t source_handle,
        LV_BooleanPtr_t set
    )
    {
        try
        {
            if(*set){
                camera_handle(camera_ref_ptr)->set_trigger(source_handle.to_utf8_string());
            }
            else{
                source_handle.copy_from_utf8(camera_handle(camera_ref_ptr)->get_trigger_source());
            }
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
}
