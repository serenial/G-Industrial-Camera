#include <cstring>
#include <cmath>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/lv_interop/lv_array_2d.hpp"

#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

namespace
{

#include "g_industrial_cam/lv_interop/set_packing.hpp"

    struct LV_BufferImageDimensions_t
    {
        uint16_t width, height;
    };

#include "g_industrial_cam/lv_interop/reset_packing.hpp"
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_image_dimensions(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_Ptr_t<LV_BufferImageDimensions_t> dims_ptr)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            dims_ptr->height = buffer.height();
            dims_ptr->width = buffer.width();
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono8_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_2DArrayHandle_t<uint8_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            std::memcpy(array_handle.begin(), buffer.begin(), buffer.size());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono10_to_16_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_2DArrayHandle_t<uint16_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            std::memcpy(array_handle.begin(), buffer.begin(), buffer.size());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono10_packed_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_2DArrayHandle_t<uint16_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            auto next_output_pixel = array_handle.begin();
            auto next_input_byte = buffer.begin();

            // process in 5 input bytes to give 3 output pixels

            while (next_output_pixel < array_handle.end() - 3 && next_input_byte < buffer.end() - 5)
            {

                auto b0 = *next_input_byte++;
                auto b1 = *next_input_byte++;
                auto b2 = *next_input_byte++;
                auto b3 = *next_input_byte++;
                auto b4 = *next_input_byte++;

                *next_output_pixel++ = static_cast<uint16_t>(b0 & 0b1111'1111) >> 0 | static_cast<uint16_t>(b1 & 0b0000'0011) << 8;
                *next_output_pixel++ = static_cast<uint16_t>(b1 & 0b1111'1100) >> 2 | static_cast<uint16_t>(b2 & 0b0000'1111) << 6;
                *next_output_pixel++ = static_cast<uint16_t>(b2 & 0b1111'0000) >> 4 | static_cast<uint16_t>(b3 & 0b0011'1111) << 4;
                *next_output_pixel++ = static_cast<uint16_t>(b3 & 0b1100'0000) >> 6 | static_cast<uint16_t>(b4 & 0b1111'1111) << 2;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_size_in_bytes(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        int32_t *size)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            *size = static_cast<int32_t>(buffer.size());
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_create_empty(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_Ptr_t<LV_BufferImageDimensions_t> dims_ptr)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr, nullptr);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}