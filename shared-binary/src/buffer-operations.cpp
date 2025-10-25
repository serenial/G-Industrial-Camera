#include <cstring>
#include <cmath>
#include <algorithm>

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
        int16_t m_x, m_y;
    };

    using LV_PictureOffsetPointPtr_t = LV_Ptr_t<LV_PictureOffsetPoint_t>;

    // Platform independent size of LV_PictureOpHeader_t when flattened by LV
    const uint8_t SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES = 28;

    enum class pixel_formats : uint8_t
    {
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

    using LV_PixmapImage_t = struct
    {
        int32_t image_type;
        int32_t image_depth;
        LV_1DArrayHandle_t<uint8_t> image_array_handle;
        LV_1DArrayHandle_t<uint8_t> mask_array_handle;
        LV_1DArrayHandle_t<uint32_t> colour_array_handle;
        int16_t left, top, right, bottom;
    };

    using LV_PixmapImagePtr_t = LV_Ptr_t<LV_PixmapImage_t>;

#include "g_industrial_cam/lv_interop/reset_packing.hpp"

    class mapped_buffer
    {
    public:
        mapped_buffer(LV_EDVRReferencePtr_t);
        ~mapped_buffer();
        void get_info(LV_MappedBufferInfo_t *) const;

    private:
        const std::unique_ptr<lv_buffer> m_buff_ptr;
    };

    const uint32_t default_colour_map[] = {0xFF000000, 0xFF010101, 0xFF020202, 0xFF030303, 0xFF040404, 0xFF050505, 0xFF060606, 0xFF070707, 0xFF080808, 0xFF090909, 0xFF0A0A0A, 0xFF0B0B0B, 0xFF0C0C0C, 0xFF0D0D0D, 0xFF0E0E0E, 0xFF0F0F0F, 0xFF101010, 0xFF111111, 0xFF121212, 0xFF131313, 0xFF141414, 0xFF151515, 0xFF161616, 0xFF171717, 0xFF181818, 0xFF191919, 0xFF1A1A1A, 0xFF1B1B1B, 0xFF1C1C1C, 0xFF1D1D1D, 0xFF1E1E1E, 0xFF1F1F1F, 0xFF202020, 0xFF212121, 0xFF222222, 0xFF232323, 0xFF242424, 0xFF252525, 0xFF262626, 0xFF272727, 0xFF282828, 0xFF292929, 0xFF2A2A2A, 0xFF2B2B2B, 0xFF2C2C2C, 0xFF2D2D2D, 0xFF2E2E2E, 0xFF2F2F2F, 0xFF303030, 0xFF313131, 0xFF323232, 0xFF333333, 0xFF343434, 0xFF353535, 0xFF363636, 0xFF373737, 0xFF383838, 0xFF393939, 0xFF3A3A3A, 0xFF3B3B3B, 0xFF3C3C3C, 0xFF3D3D3D, 0xFF3E3E3E, 0xFF3F3F3F, 0xFF404040, 0xFF414141, 0xFF424242, 0xFF434343, 0xFF444444, 0xFF454545, 0xFF464646, 0xFF474747, 0xFF484848, 0xFF494949, 0xFF4A4A4A, 0xFF4B4B4B, 0xFF4C4C4C, 0xFF4D4D4D, 0xFF4E4E4E, 0xFF4F4F4F, 0xFF505050, 0xFF515151, 0xFF525252, 0xFF535353, 0xFF545454, 0xFF555555, 0xFF565656, 0xFF575757, 0xFF585858, 0xFF595959, 0xFF5A5A5A, 0xFF5B5B5B, 0xFF5C5C5C, 0xFF5D5D5D, 0xFF5E5E5E, 0xFF5F5F5F, 0xFF606060, 0xFF616161, 0xFF626262, 0xFF636363, 0xFF646464, 0xFF656565, 0xFF666666, 0xFF676767, 0xFF686868, 0xFF696969, 0xFF6A6A6A, 0xFF6B6B6B, 0xFF6C6C6C, 0xFF6D6D6D, 0xFF6E6E6E, 0xFF6F6F6F, 0xFF707070, 0xFF717171, 0xFF727272, 0xFF737373, 0xFF747474, 0xFF757575, 0xFF767676, 0xFF777777, 0xFF787878, 0xFF797979, 0xFF7A7A7A, 0xFF7B7B7B, 0xFF7C7C7C, 0xFF7D7D7D, 0xFF7E7E7E, 0xFF7F7F7F, 0xFF808080, 0xFF818181, 0xFF828282, 0xFF838383, 0xFF848484, 0xFF858585, 0xFF868686, 0xFF878787, 0xFF888888, 0xFF898989, 0xFF8A8A8A, 0xFF8B8B8B, 0xFF8C8C8C, 0xFF8D8D8D, 0xFF8E8E8E, 0xFF8F8F8F, 0xFF909090, 0xFF919191, 0xFF929292, 0xFF939393, 0xFF949494, 0xFF959595, 0xFF969696, 0xFF979797, 0xFF989898, 0xFF999999, 0xFF9A9A9A, 0xFF9B9B9B, 0xFF9C9C9C, 0xFF9D9D9D, 0xFF9E9E9E, 0xFF9F9F9F, 0xFFA0A0A0, 0xFFA1A1A1, 0xFFA2A2A2, 0xFFA3A3A3, 0xFFA4A4A4, 0xFFA5A5A5, 0xFFA6A6A6, 0xFFA7A7A7, 0xFFA8A8A8, 0xFFA9A9A9, 0xFFAAAAAA, 0xFFABABAB, 0xFFACACAC, 0xFFADADAD, 0xFFAEAEAE, 0xFFAFAFAF, 0xFFB0B0B0, 0xFFB1B1B1, 0xFFB2B2B2, 0xFFB3B3B3, 0xFFB4B4B4, 0xFFB5B5B5, 0xFFB6B6B6, 0xFFB7B7B7, 0xFFB8B8B8, 0xFFB9B9B9, 0xFFBABABA, 0xFFBBBBBB, 0xFFBCBCBC, 0xFFBDBDBD, 0xFFBEBEBE, 0xFFBFBFBF, 0xFFC0C0C0, 0xFFC1C1C1, 0xFFC2C2C2, 0xFFC3C3C3, 0xFFC4C4C4, 0xFFC5C5C5, 0xFFC6C6C6, 0xFFC7C7C7, 0xFFC8C8C8, 0xFFC9C9C9, 0xFFCACACA, 0xFFCBCBCB, 0xFFCCCCCC, 0xFFCDCDCD, 0xFFCECECE, 0xFFCFCFCF, 0xFFD0D0D0, 0xFFD1D1D1, 0xFFD2D2D2, 0xFFD3D3D3, 0xFFD4D4D4, 0xFFD5D5D5, 0xFFD6D6D6, 0xFFD7D7D7, 0xFFD8D8D8, 0xFFD9D9D9, 0xFFDADADA, 0xFFDBDBDB, 0xFFDCDCDC, 0xFFDDDDDD, 0xFFDEDEDE, 0xFFDFDFDF, 0xFFE0E0E0, 0xFFE1E1E1, 0xFFE2E2E2, 0xFFE3E3E3, 0xFFE4E4E4, 0xFFE5E5E5, 0xFFE6E6E6, 0xFFE7E7E7, 0xFFE8E8E8, 0xFFE9E9E9, 0xFFEAEAEA, 0xFFEBEBEB, 0xFFECECEC, 0xFFEDEDED, 0xFFEEEEEE, 0xFFEFEFEF, 0xFFF0F0F0, 0xFFF1F1F1, 0xFFF2F2F2, 0xFFF3F3F3, 0xFFF4F4F4, 0xFFF5F5F5, 0xFFF6F6F6, 0xFFF7F7F7, 0xFFF8F8F8, 0xFFF9F9F9, 0xFFFAFAFA, 0xFFFBFBFB, 0xFFFCFCFC, 0xFFFDFDFD, 0xFFFEFEFE, 0xFFFFFFFF};

    void copy_buffer_data_with_lv_picture_packing(const pixel_formats format, const lv_buffer &buffer, uint8_t *dst_data_ptr, const size_t dst_data_ptr_size);
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

            std::memcpy(array_handle.begin(), buffer.begin(), buffer.height() * buffer.width());
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

            std::memcpy(array_handle.begin(), buffer.begin(), buffer.height() * buffer.width());
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

            while (input_byte + 5 <= buffer.end() && output_pixel + 4 <= array_handle.end())
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

            while (input_byte + 3 <= buffer.end() && output_pixel + 2 <= array_handle.end())
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

            // process in 7 input bytes to give 4 output pixels

            while (input_byte + 7 <= buffer.end() && output_pixel + 4 <= array_handle.end())
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
                *output_pixel++ = static_cast<uint16_t>(b5 & 0b1111'1100) >> 2 | static_cast<uint16_t>(b6 & 0b1111'1111) << 6;
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

            while (input_byte + 3 <= buffer.end() && output_pixel < array_handle.end())
            {

                auto red = *input_byte++;
                auto green = *input_byte++;
                auto blue = *input_byte++;

                if (*flip_rb_channels)
                {
                    *output_pixel = alpha_channel_value << 24 | static_cast<uint32_t>(blue) << 16 | static_cast<uint32_t>(green) << 8 | static_cast<uint32_t>(red);
                }
                else
                {
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
        uint8_t pixel_format)
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

            if (is_greyscale)
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

            if (buffer.size() == 0)
            {
                return LV_ERR_noError;
            }

            // get a pointer to the point in the "string" bytes where the rgb pixel data starts
            auto dst_data_ptr = lv_str_handle.begin() + SIZEOF_LV_FLATTENED_PICTURE_OP_HEADER_BYTES;

            if (is_greyscale)
            {
                op_header_ptr->length = image_bytes_length + 22 + lv_picture::greyscale_lookup_length + mask_bytes_length;

                // copy the greyscale colour-table into string
                std::memcpy(dst_data_ptr, lv_picture::greyscale_lookup, lv_picture::greyscale_lookup_length);

                dst_data_ptr += lv_picture::greyscale_lookup_length;

                copy_buffer_data_with_lv_picture_packing(format, buffer, reinterpret_cast<uint8_t *>(dst_data_ptr), image_bytes_length);
            }
            else
            {

                op_header_ptr->length = image_bytes_length + mask_bytes_length + 24; // 24 = 22 + 2 bytes for the empty mask

                // set the first two bytes to zero which is a uint16_t value representing that the colour table is not used (length 0)

                for (size_t i = 0; i < 2; i++)
                {
                    *dst_data_ptr = 0;
                    ++dst_data_ptr;
                }

                copy_buffer_data_with_lv_picture_packing(format, buffer, reinterpret_cast<uint8_t *>(dst_data_ptr), image_bytes_length);
            }
        }
        catch (...)
        {
            error_cluster_ptr.copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_read_as_pixmap(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        LV_PictureOffsetPointPtr_t top_left_ptr,
        LV_1DArrayHandle_t<uint8_t> mask_array_handle,
        LV_1DArrayHandle_t<uint32_t> colour_array_handle,
        LV_PixmapImagePtr_t pixmap_ptr,
        uint8_t pixel_format)
    {
        try
        {

            lv_buffer buffer(edvr_ref_ptr);
            auto format = static_cast<pixel_formats>(pixel_format);
            auto is_greyscale = format != pixel_formats::BGR8 && format != pixel_formats::RGB8;

            int32_t output_width = buffer.width();

            auto width_is_even = output_width % 2 == 0;

            if (is_greyscale && !width_is_even)
            {
                ++output_width;
            }

            size_t image_bytes_length;

            if (is_greyscale)
            {
                image_bytes_length = output_width * buffer.height();
            }
            else
            {
                image_bytes_length = output_width * buffer.height() * 3;
            }

            // size pixmap image_data
            pixmap_ptr->image_array_handle.size_to_fit(image_bytes_length);

            pixmap_ptr->top = top_left_ptr->m_y;
            pixmap_ptr->left = top_left_ptr->m_x;
            pixmap_ptr->bottom = pixmap_ptr->top + buffer.height();
            pixmap_ptr->right = pixmap_ptr->left + buffer.width();

            if (buffer.size() == 0)
            {
                return LV_ERR_noError;
            }

            if (is_greyscale)
            {
                pixmap_ptr->image_depth = 8;

                // handle colour map
                if (colour_array_handle.empty())
                {
                    // assume that a non-empty pixmap colour_array has already been populated with the correct values
                    if (pixmap_ptr->colour_array_handle.empty())
                    {
                        pixmap_ptr->colour_array_handle.size_to_fit(256);
                        std::memcpy(pixmap_ptr->colour_array_handle.begin(), &default_colour_map[0], pixmap_ptr->colour_array_handle.size() * sizeof(uint32_t));
                    }
                }
                else
                {
                    pixmap_ptr->colour_array_handle.size_to_fit(std::min(colour_array_handle.size(), size_t(256)));
                    std::memcpy(pixmap_ptr->colour_array_handle.begin(), colour_array_handle.begin(), pixmap_ptr->colour_array_handle.size() * sizeof(uint32_t));
                }
            }
            else
            {
                pixmap_ptr->image_depth = 24;
            }

            copy_buffer_data_with_lv_picture_packing(format, buffer, pixmap_ptr->image_array_handle.begin(), image_bytes_length);

            pixmap_ptr->image_type = 0;
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
        LV_Ptr_t<LV_MappedBufferInfo_t> mapped_info_ptr)
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

void mapped_buffer::get_info(LV_MappedBufferInfo_t *info) const
{
    info->res_x = m_buff_ptr->width();
    info->res_y = m_buff_ptr->height();
    info->number_of_bytes = m_buff_ptr->size();
}

namespace
{
    // copy buffer data into the format expected by lv pictures and pixmap types
    // dst_data_ptr must be sized to accommodate the data first!
    void copy_buffer_data_with_lv_picture_packing(const pixel_formats format, const lv_buffer &buffer, uint8_t *dst_data_ptr, const size_t dst_data_ptr_size)
    {
        size_t pixel = 0;
        auto src_data_ptr = buffer.begin();
        bool width_is_even = buffer.width() % 2 == 0;

        auto check_and_handle_odd_width = [&]()
        {
            ++pixel;
            if (!width_is_even && pixel == buffer.width())
            {
                ++dst_data_ptr; // jump extra pixel at end of row
                pixel = 0;
            }
        };

        const auto dst_end = dst_data_ptr + dst_data_ptr_size;

        switch (format)
        {
        case pixel_formats::mono8:
            if (width_is_even)
            {
                // direct copy
                std::memcpy(dst_data_ptr, src_data_ptr, dst_data_ptr_size);
            }
            else
            {
                // line-by-line copy

                while (src_data_ptr < buffer.end() && dst_data_ptr + buffer.width() <= dst_end)
                {
                    std::memcpy(dst_data_ptr, src_data_ptr, buffer.width());
                    src_data_ptr += buffer.width();
                    dst_data_ptr += buffer.width() + 1; // jump over the extra pixel
                }
            }
            break;
        case pixel_formats::mono10:
            // pixel by pixel copy
            while (src_data_ptr + 2 <= buffer.end() && dst_data_ptr < dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 | (*src_data_ptr++ & 0b0000'0011) << 6;
                check_and_handle_odd_width();
            }
            break;
        case pixel_formats::mono12:
            // pixel by pixel copy
            while (src_data_ptr + 2 <= buffer.end() && dst_data_ptr < dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr++ & 0b0000'1111) << 4;
                check_and_handle_odd_width();
            }
            break;
        case pixel_formats::mono14:
            // pixel by pixel copy
            while (src_data_ptr + 2 <= buffer.end() && dst_data_ptr < dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 | (*src_data_ptr++ & 0b0011'1111) << 2;
                check_and_handle_odd_width();
            }
            break;
        case pixel_formats::mono16:
            // pixel by pixel copy
            while (src_data_ptr + 2 <= buffer.end() && dst_data_ptr < dst_end)
            {
                // just take the higher bytes
                ++src_data_ptr;
                *dst_data_ptr++ = *(src_data_ptr)++;
                check_and_handle_odd_width();
            }
            break;
        case pixel_formats::mono10p:
            // pixel by pixel copy
            while (src_data_ptr + 5 <= buffer.end() && dst_data_ptr + 4 <= dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'1100) >> 2 | (*src_data_ptr & 0b0000'0011) << 6;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr & 0b0000'1111) << 4;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 | (*src_data_ptr & 0b0011'1111) << 2;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 | (*src_data_ptr & 0b1111'1111) << 0;
                check_and_handle_odd_width();
                ++src_data_ptr;
            }
            break;
        case pixel_formats::mono12p:
            // pixel by pixel copy
            while (src_data_ptr + 3 <= buffer.end() && dst_data_ptr + 2 <= dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr & 0b0000'1111) << 4;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 | (*src_data_ptr & 0b1111'1111) << 0;
                check_and_handle_odd_width();
                ++src_data_ptr;
            }
            break;
        case pixel_formats::mono14p:
            // pixel by pixel copy
            while (src_data_ptr + 7 <= buffer.end() && dst_data_ptr + 4 <= dst_end)
            {
                // combine 8 msb bits into single output byte
                *dst_data_ptr++ = (*src_data_ptr++ & 0b1100'0000) >> 6 | (*src_data_ptr & 0b0011'1111) << 2;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 | (*src_data_ptr++ & 0b1111'0000) >> 4 | (*src_data_ptr & 0b0000'1111) << 4;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 | (*src_data_ptr++ & 0b1111'1100) >> 2 | (*src_data_ptr & 0b0000'0011) << 6;
                check_and_handle_odd_width();
                *dst_data_ptr++ = (*src_data_ptr++ & 0b0000'0000) >> 0 | (*src_data_ptr & 0b1111'1111) << 0;
                check_and_handle_odd_width();
                ++src_data_ptr;
            }
            break;
        case pixel_formats::RGB8:
            // straight copy
            std::memcpy(dst_data_ptr, buffer.begin(), dst_data_ptr_size);
            break;
        case pixel_formats::BGR8:
        {
            auto src_data_ptr = buffer.begin();
            // copy with pixel byte order swapped
            while (src_data_ptr + 3 <= buffer.end() && dst_data_ptr + 3 <= dst_end)
            {
                *(dst_data_ptr + 0) = *(src_data_ptr + 2);
                *(dst_data_ptr + 1) = *(src_data_ptr + 1);
                *(dst_data_ptr + 2) = *(src_data_ptr + 0);
                src_data_ptr += 3;
                dst_data_ptr += 3;
            }
        }
        break;
        default:
            throw std::runtime_error("Unsupported pixel format type.");
        }
    }
}