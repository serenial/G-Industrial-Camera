#include <cstring>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/device-io/buffer.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

namespace{
    using buffer_handle = EDVRManagedObject<buffer>;

    #include "g_industrial_cam/lv_interop/set_packing.hpp"

    struct LV_BufferImageDimensions_t{
        uint16_t width, height;
    };


    #include "g_industrial_cam/lv_interop/reset_packing.hpp"
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_raw(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_StringHandle_t raw_handle
    )
    {
        try
        {
            buffer_handle buffer(edvr_ref_ptr);

            raw_handle.copy_from(buffer->begin(), buffer->size());

        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_image_dimensions(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_Ptr_t<LV_BufferImageDimensions_t> dims_ptr
    )
    {
        try
        {
            buffer_handle buffer(edvr_ref_ptr);

            dims_ptr->height = buffer->height();
            dims_ptr->width = buffer->width();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}