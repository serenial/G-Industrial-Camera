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

    struct LV_MappedBufferInfo_t
    {
        int32_t res_x, res_y;
        uint64_t number_of_bytes;
    };


#include "g_industrial_cam/lv_interop/reset_packing.hpp"


class mapped_buffer
{
public:
    mapped_buffer(LV_EDVRReferencePtr_t);
    ~mapped_buffer();
    void get_info(LV_MappedBufferInfo_t*) const;

private:
    const std::unique_ptr<lv_buffer> m_buff_ptr;
};

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

            auto output_pixel = array_handle.begin();
            auto input_byte = buffer.begin();

            // process in 5 input bytes to give 3 output pixels

            while (output_pixel < array_handle.end() && input_byte < buffer.end())
            {

                auto b0 = *input_byte++;
                auto b1 = *input_byte++;
                auto b2 = *input_byte++;
                auto b3 = *input_byte++;
                auto b4 = *input_byte++;

                *output_pixel++ = static_cast<uint16_t>(b0 & 0b1111'1111) >> 0 | static_cast<uint16_t>(b1 & 0b0000'0011) << 8;
                *output_pixel++ = static_cast<uint16_t>(b1 & 0b1111'1100) >> 2 | static_cast<uint16_t>(b2 & 0b0000'1111) << 6;
                *output_pixel++ = static_cast<uint16_t>(b2 & 0b1111'0000) >> 4 | static_cast<uint16_t>(b3 & 0b0011'1111) << 4;
                *output_pixel++ = static_cast<uint16_t>(b3 & 0b1100'0000) >> 6 | static_cast<uint16_t>(b4 & 0b1111'1111) << 2;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono12_packed_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_2DArrayHandle_t<uint16_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            auto output_pixel = array_handle.begin();
            auto input_byte = buffer.begin();

            // process in 3 input bytes to give 2 output pixels

            while (output_pixel < array_handle.end() && input_byte < buffer.end())
            {

                auto b0 = *input_byte++;
                auto b1 = *input_byte++;
                auto b2 = *input_byte++;

                *output_pixel++ = static_cast<uint16_t>(b0 & 0b1111'1111) >> 0 | static_cast<uint16_t>(b1 & 0b0000'1111) << 8;
                *output_pixel++ = static_cast<uint16_t>(b1 & 0b1111'0000) >> 4 | static_cast<uint16_t>(b2 & 0b1111'1111) << 4;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono14_packed_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_2DArrayHandle_t<uint16_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            auto output_pixel = array_handle.begin();
            auto input_byte = buffer.begin();

            // process in 6 input bytes to give 4 output pixels

            while (output_pixel < array_handle.end() && input_byte < buffer.end())
            {

                auto b0 = *input_byte++;
                auto b1 = *input_byte++;
                auto b2 = *input_byte++;
                auto b3 = *input_byte++;
                auto b4 = *input_byte++;
                auto b5 = *input_byte++;
                auto b6 = *input_byte++;

                *output_pixel++ = static_cast<uint16_t>(b0 & 0b1111'1111) >> 0 | static_cast<uint16_t>(b1 & 0b0011'1111) << 8;
                *output_pixel++ = static_cast<uint16_t>(b1 & 0b1100'0000) >> 6 | static_cast<uint16_t>(b2 & 0b1111'1111) << 2 | static_cast<uint16_t>(b3 & 0b0000'1111) << 10;
                *output_pixel++ = static_cast<uint16_t>(b3 & 0b1111'0000) >> 4 | static_cast<uint16_t>(b4 & 0b1111'1111) << 4 | static_cast<uint16_t>(b5 & 0b0000'0011) << 12;
                *output_pixel++ = static_cast<uint16_t>(b5 & 0b1111'1100) >> 2 | static_cast<uint16_t>(b6 & 0b1111'1111) << 6 ;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

        G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_rgb8_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_BooleanPtr_t flip_rb_channels,
        uint8_t alpha_channel_value,
        LV_2DArrayHandle_t<uint32_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            auto output_pixel = array_handle.begin();
            auto input_byte = buffer.begin();

            // process in 3 input bytes to give 1 output pixel

            while (output_pixel < array_handle.end() && input_byte < buffer.end())
            {

                auto red = *input_byte++; // r
                auto green = *input_byte++; // g
                auto blue = *input_byte++; // b

                if(*flip_rb_channels){
                    *output_pixel = alpha_channel_value << 24 | static_cast<uint32_t>(blue) << 16 | static_cast<uint32_t>(green) << 8 | static_cast<uint32_t>(red); 
                }
                else{
                    *output_pixel = alpha_channel_value << 24 | static_cast<uint32_t>(red) << 16 | static_cast<uint32_t>(green) << 8 | static_cast<uint32_t>(blue); 
                }
               output_pixel++;
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

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_map_buffer(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_EDVRReferencePtr_t mapped_lifetime_edvr_ref_ptr,
        LV_Ptr_t<LV_MappedBufferInfo_t> mapped_info_ptr
    )
    {
        try
        {
            EDVRManagedObject<mapped_buffer> mapped(mapped_lifetime_edvr_ref_ptr, new mapped_buffer(edvr_ref_ptr));

            mapped->get_info(mapped_info_ptr);
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }

        return LV_ERR_noError;
    }
}

mapped_buffer::mapped_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr)
    : m_buff_ptr(std::make_unique<lv_buffer>(edvr_ref_ptr))
{
    m_buff_ptr->upgrade_to_mapped();
}

mapped_buffer::~mapped_buffer()
{
    m_buff_ptr->downgrade_from_mapped();
}

void mapped_buffer::get_info(LV_MappedBufferInfo_t* info) const
{
    info->res_x = m_buff_ptr->width();
    info->res_y = m_buff_ptr->height();
    info->number_of_bytes = m_buff_ptr->size();
}