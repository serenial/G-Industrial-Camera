#include <memory>
#include <arv.h>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_array_2d.hpp"
#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_snapshot(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_2DArrayHandle_t<uint8_t> handle)
    {
        try
        {
            std::unique_ptr<ArvCamera, void(*)(ArvCamera*)> camera(arv_camera_new(NULL, NULL), [](ArvCamera* ptr){
                g_clear_object(&ptr);
            });
            std::unique_ptr<ArvBuffer, void(*)(ArvBuffer*)> buffer(arv_camera_acquisition(camera.get(), 0, NULL), [](ArvBuffer* ptr){
                g_clear_object(&ptr);
            });

            if (ARV_IS_BUFFER(buffer.get()) && arv_buffer_get_status(buffer.get()) == ARV_BUFFER_STATUS_SUCCESS)
            {
                auto width = arv_buffer_get_image_height(buffer.get());
                auto height = arv_buffer_get_image_width(buffer.get());
                handle.size_to_fit({width, height});

                std::memcpy(handle.at({0,0}), arv_buffer_get_image_data(buffer.get(), NULL), width * height);
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}