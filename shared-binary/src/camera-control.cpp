#include <stdexcept>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/device-io/camera.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

namespace{
    using camera_handle = EDVRManagedObject<camera>;
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_create(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_StringHandle_t id_handle,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            camera_handle(edvr_ref_ptr, camera::create(id_handle.to_utf8_string()));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_avaliable_pixel_formats(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_1DArrayHandle_t<LV_StringHandle_t> formats_handle)
    {
        try
        {
            std::vector<std::string> formats;
            camera_handle(edvr_ref_ptr)->get_avaliable_pixel_formats(formats);

            formats_handle.copy_element_by_element_from(formats, [](auto from, auto to)
                                                        { to->copy_from_utf8(from); });
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_camera_get_set_pixel_format(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t format_handle,
        LV_BooleanPtr_t set)
    {
        try
        {
            camera_handle cam(edvr_ref_ptr);

            if (*set)
            {
                cam->set_pixel_format(format_handle.to_utf8_string());
            }
            else
            {
                format_handle.copy_from_utf8(cam->get_pixel_format());
            }
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

            auto buf = camera_handle(camera_ref_ptr)->take_snapshot(timeout_us);

            if (arv_buffer_get_status(buf) != ARV_BUFFER_STATUS_SUCCESS)
            {
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
}
