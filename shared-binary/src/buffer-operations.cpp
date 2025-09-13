#include <cstring>
#include <cmath>

#include "g_industrial_cam/lv_interop/lv_types.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"
#include "g_industrial_cam/lv_interop/lv_array_2d.hpp"
#include "g_industrial_cam/lv_interop/lv_picture.hpp"

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

    struct LV_PictureOpHeader_t
    {
        int16_t op_code;
        int32_t length, image_bytes_length;
        int16_t top, left, bottom, right;
        int16_t bitwidth;
        uint32_t default_foreground, default_background;
    };

    using LV_PictureOpHeaderPtr_t = LV_Ptr_t<LV_PictureOpHeader_t>;

    struct LV_PictureOffsetPoint_t
    {
        int16_t m_x,m_y;
    };

    using LV_PictureOffsetPointPtr_t = LV_Ptr_t<LV_PictureOffsetPoint_t>;

    // Platform independent size of LV_PictureOpHeader_t when flattened by LV
    const uint8_t SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES = 28;

    enum class pixel_formats : uint8_t {
        mono8,
        mono10,
        mono10p,
        mono12,
        mono12p,
        mono14,
        mono14p,
        mono16,
        RGB8,
        BGR8
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
               ++output_pixel;
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_draw_lv_picture(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_PictureOffsetPointPtr_t top_left_ptr,
        LV_StringHandle_t lv_str_handle,
        LV_PictureOpHeaderPtr_t op_header_ptr,
        uint8_t pixel_format
    )
    {
        try
        {

            lv_buffer buffer(edvr_ref_ptr);
            auto format = static_cast<pixel_formats>(pixel_format);
            auto is_greyscale = format != pixel_formats::BGR8 && format != pixel_formats::RGB8;
            
            int32_t output_width = buffer.width();

            size_t mask_row_length = 0, mask_bytes_length = 0;

            auto width_is_even = output_width % 2 == 0;

            if (is_greyscale && !width_is_even)
            {
                ++output_width;
            }

            size_t image_bytes_length;
            size_t total_data_length;

            if(is_greyscale)
            {
                image_bytes_length = output_width * buffer.height();
                total_data_length = SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES + image_bytes_length + lv_picture::greyscale_lookup_length;
            }
            else
            {
                image_bytes_length = output_width * buffer.height() * 3;
                total_data_length = SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES + 2 + image_bytes_length; // add sizeof uint16_t to account for unused colour table
            }

            // size source string-handle to hold our image data
            lv_str_handle.size_to_fit(total_data_length);

            // use some magic numbers to set the header
            op_header_ptr->op_code = 29; // draw rgb data
            op_header_ptr->image_bytes_length = image_bytes_length;
            op_header_ptr->bitwidth = is_greyscale ? 8 : 24;
            op_header_ptr->default_background = 0x00FFFFFF; // white
            op_header_ptr->default_foreground = 0x00000000; // black
            op_header_ptr->top = top_left_ptr->m_y;
            op_header_ptr->left = top_left_ptr->m_x;
            op_header_ptr->bottom = op_header_ptr->top + buffer.height();
            op_header_ptr->right = op_header_ptr->left + buffer.width();


            if(buffer.size() == 0){
                return LV_ERR_noError;
            }

            // get a pointer to the point in the "string" bytes where the rgb pixel data starts
            auto dst_data_ptr = lv_str_handle.begin() + SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES;

            if(is_greyscale){
                op_header_ptr->length = image_bytes_length + 22 + lv_picture::greyscale_lookup_length + mask_bytes_length;

                // copy the greyscale colour-table into string
                std::memcpy(dst_data_ptr, lv_picture::greyscale_lookup, lv_picture::greyscale_lookup_length);

                dst_data_ptr += lv_picture::greyscale_lookup_length;
                
                size_t pixel = 0;
                auto src_data_ptr = buffer.begin();

                auto check_and_handle_odd_width = [&](){
                    ++pixel;
                    if(!width_is_even && pixel == buffer.width()){
                                ++dst_data_ptr; // jump extra pixel at end of row
                                pixel = 0;
                    }
                };

                switch(format){
                    case pixel_formats::mono8 :
                        if(width_is_even){
                            //direct copy
                            std::memcpy(dst_data_ptr, src_data_ptr, buffer.size());
                        }
                        else{
                            // line-by-line copy

                            while(src_data_ptr < buffer.end()){
                                std::memcpy(dst_data_ptr,src_data_ptr, buffer.width());
                                src_data_ptr += buffer.width();
                                dst_data_ptr += buffer.width() + 1; // jump over the extra pixel
                            }
                        }
                    break;
                    case pixel_formats::mono10 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 |  (*src_data_ptr++ & 0b0000'0011) << 6;
                            check_and_handle_odd_width();

                        }
                    break;
                    case pixel_formats::mono12 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr++ & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                        }
                    break;
                    case pixel_formats::mono14 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr++ & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                        }
                    break;
                    case pixel_formats::mono16 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // just take the higher bytes
                            ++src_data_ptr;
                            *dst_data_ptr++ = *(src_data_ptr)++;
                            check_and_handle_odd_width();
                        }
                    break;
                        case pixel_formats::mono10p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 |  (*src_data_ptr & 0b0000'0011) << 6;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0;
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                        case pixel_formats::mono12p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0;
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                        case pixel_formats::mono14p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr & 0b0000'1111) << 4 ; 
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr++ & 0b1111'1100) >> 2 | (*src_data_ptr & 0b0000'0011) << 6 ; 
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0; 
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                }

            }
            else{

                op_header_ptr->length = image_bytes_length + mask_bytes_length + 24; // 24 = 22 + 2 bytes for the empty mask

                // set the first two bytes to zero which is a uint16_t value representing that the colour table is not used (length 0)

                for (size_t i = 0; i < 2; i++)
                {
                    *dst_data_ptr = 0;
                    ++dst_data_ptr;
                }

                switch (format){
                    case pixel_formats::RGB8 :
                        // straight copy
                        std::memcpy(dst_data_ptr, buffer.begin(), buffer.size());
                    break;
                    case pixel_formats::BGR8 :
                        auto src_data_ptr = buffer.begin();
                        // copy with pixel byte order swapped
                        while(src_data_ptr < buffer.end()){
                            *(dst_data_ptr + 0) = *(src_data_ptr + 2);
                            *(dst_data_ptr + 1) = *(src_data_ptr + 1);
                            *(dst_data_ptr + 2) = *(src_data_ptr + 0);
                            src_data_ptr +=3;
                            dst_data_ptr +=3;
                        }
                    break;
                }
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_draw_lv_picture(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_PictureOffsetPointPtr_t top_left_ptr,
        LV_StringHandle_t lv_str_handle,
        LV_PictureOpHeaderPtr_t op_header_ptr,
        uint8_t pixel_format
    )
    {
        try
        {

            lv_buffer buffer(edvr_ref_ptr);
            auto format = static_cast<pixel_formats>(pixel_format);
            auto is_greyscale = format != pixel_formats::BGR8 && format != pixel_formats::RGB8;
            
            int32_t output_width = buffer.width();

            size_t mask_row_length = 0, mask_bytes_length = 0;

            auto width_is_even = output_width % 2 == 0;

            if (is_greyscale && !width_is_even)
            {
                ++output_width;
            }

            size_t image_bytes_length;
            size_t total_data_length;

            if(is_greyscale)
            {
                image_bytes_length = output_width * buffer.height();
                total_data_length = SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES + image_bytes_length + lv_picture::greyscale_lookup_length;
            }
            else
            {
                image_bytes_length = output_width * buffer.height() * 3;
                total_data_length = SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES + 2 + image_bytes_length; // add sizeof uint16_t to account for unused colour table
            }

            // size source string-handle to hold our image data
            lv_str_handle.size_to_fit(total_data_length);

            // use some magic numbers to set the header
            op_header_ptr->op_code = 29; // draw rgb data
            op_header_ptr->image_bytes_length = image_bytes_length;
            op_header_ptr->bitwidth = is_greyscale ? 8 : 24;
            op_header_ptr->default_background = 0x00FFFFFF; // white
            op_header_ptr->default_foreground = 0x00000000; // black
            op_header_ptr->top = top_left_ptr->m_y;
            op_header_ptr->left = top_left_ptr->m_x;
            op_header_ptr->bottom = op_header_ptr->top + buffer.height();
            op_header_ptr->right = op_header_ptr->left + buffer.width();


            if(buffer.size() == 0){
                return LV_ERR_noError;
            }

            // get a pointer to the point in the "string" bytes where the rgb pixel data starts
            auto dst_data_ptr = lv_str_handle.begin() + SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES;

            if(is_greyscale){
                op_header_ptr->length = image_bytes_length + 22 + lv_picture::greyscale_lookup_length + mask_bytes_length;

                // copy the greyscale colour-table into string
                std::memcpy(dst_data_ptr, lv_picture::greyscale_lookup, lv_picture::greyscale_lookup_length);

                dst_data_ptr += lv_picture::greyscale_lookup_length;
                
                size_t pixel = 0;
                auto src_data_ptr = buffer.begin();

                auto check_and_handle_odd_width = [&](){
                    ++pixel;
                    if(!width_is_even && pixel == buffer.width()){
                                ++dst_data_ptr; // jump extra pixel at end of row
                                pixel = 0;
                    }
                };

                switch(format){
                    case pixel_formats::mono8 :
                        if(width_is_even){
                            //direct copy
                            std::memcpy(dst_data_ptr, src_data_ptr, buffer.size());
                        }
                        else{
                            // line-by-line copy

                            while(src_data_ptr < buffer.end()){
                                std::memcpy(dst_data_ptr,src_data_ptr, buffer.width());
                                src_data_ptr += buffer.width();
                                dst_data_ptr += buffer.width() + 1; // jump over the extra pixel
                            }
                        }
                    break;
                    case pixel_formats::mono10 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 |  (*src_data_ptr++ & 0b0000'0011) << 6;
                            check_and_handle_odd_width();

                        }
                    break;
                    case pixel_formats::mono12 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr++ & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                        }
                    break;
                    case pixel_formats::mono14 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr++ & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                        }
                    break;
                    case pixel_formats::mono16 :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // just take the higher bytes
                            ++src_data_ptr;
                            *dst_data_ptr++ = *(src_data_ptr)++;
                            check_and_handle_odd_width();
                        }
                    break;
                        case pixel_formats::mono10p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 |  (*src_data_ptr & 0b0000'0011) << 6;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0;
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                        case pixel_formats::mono12p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 |  (*src_data_ptr & 0b0000'1111) << 4;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0;
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                        case pixel_formats::mono14p :
                        // pixel by pixel copy
                        while(src_data_ptr < buffer.end()){
                            // combine 8 msb bits into single output byte
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 |  (*src_data_ptr & 0b0011'1111) << 2;
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr & 0b0000'1111) << 4 ; 
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr++ & 0b1111'1100) >> 2 | (*src_data_ptr & 0b0000'0011) << 6 ; 
                            check_and_handle_odd_width();
                            *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 |  (*src_data_ptr & 0b1111'1111) << 0; 
                            check_and_handle_odd_width();
                            ++src_data_ptr;
                        }
                }

            }
            else{

                op_header_ptr->length = image_bytes_length + mask_bytes_length + 24; // 24 = 22 + 2 bytes for the empty mask

                // set the first two bytes to zero which is a uint16_t value representing that the colour table is not used (length 0)

                for (size_t i = 0; i < 2; i++)
                {
                    *dst_data_ptr = 0;
                    ++dst_data_ptr;
                }

                switch (format){
                    case pixel_formats::RGB8 :
                        // straight copy
                        std::memcpy(dst_data_ptr, buffer.begin(), buffer.size());
                    break;
                    case pixel_formats::BGR8 :
                        auto src_data_ptr = buffer.begin();
                        // copy with pixel byte order swapped
                        while(src_data_ptr < buffer.end()){
                            *(dst_data_ptr + 0) = *(src_data_ptr + 2);
                            *(dst_data_ptr + 1) = *(src_data_ptr + 1);
                            *(dst_data_ptr + 2) = *(src_data_ptr + 0);
                            src_data_ptr +=3;
                            dst_data_ptr +=3;
                        }
                    break;
                }
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