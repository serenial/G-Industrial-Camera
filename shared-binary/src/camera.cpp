#include <stdexcept>
#include <format>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/camera.hpp"
#include "g_industrial_cam/aravis-error.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

camera::camera(const std::string& identifier) : m_camera(nullptr),
                                         m_is_streaming(false)
{
    aravis_error err;
    auto cam = arv_camera_new(identifier.c_str(), err);

    if(!ARV_IS_CAMERA(cam)){
        throw std::invalid_argument("Unable to find a matching camera.");
    }
    
    aravis_error::check_error(err);

    m_camera =  cam;
}

camera::~camera()
{
    if(m_camera!=nullptr){
        g_clear_object(&m_camera);
    }
    m_camera = nullptr;
}

buffer *camera::take_snapshot(uint32_t timeout) const
{

    aravis_error err;

    /* Acquire a single buffer */
    ArvBuffer *buf = arv_camera_acquisition(m_camera, timeout, err);
    
    aravis_error::check_error(err);

    return new buffer(buf);
}

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