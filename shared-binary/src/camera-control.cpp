#include <stdexcept>
#include <format>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/device-io/camera.hpp"
#include "g_industrial_cam/device-io/aravis-error.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_create(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_StringHandle_t id_handle,
        LV_EDVRReferencePtr_t edvr_ref_ptr)
    {
        try
        {
            EDVRManagedObject<camera>(edvr_ref_ptr, new camera(id_handle.to_utf8_string()));
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_take_snapshot(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_StringHandle_t id_handle,
        LV_EDVRReferencePtr_t camera_ref_ptr,
        uint32_t timeout,
        LV_EDVRReferencePtr_t buffer_ref_ptr
    )
    {
        try
        {
            EDVRManagedObject<camera> cam(camera_ref_ptr);

            auto buf = cam->take_snapshot(timeout);

            if(!buf->has_status_success()){
                return LV_ERR_ncTimeOutErr;
            }

            EDVRManagedObject<buffer>(buffer_ref_ptr, buf);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}
