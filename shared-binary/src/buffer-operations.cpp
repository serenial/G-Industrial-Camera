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

// this lookup is really a sparse matrix implemented as a static 2D array as it is only small
// we don't need to worry that some of the values will never be used
// use lookup as follows arr[number-of-bits(1 to 8) - 1][offset of first bit (0 to 7)]

static uint8_t mono_packed_masks_lookup [8][8] = {
                                                  { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 },
                                                  { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00 },
                                                  { 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0x00, 0x00 },
                                                  { 0x0F, 0x1E, 0x3C, 0x78, 0xF0, 0x00, 0x00, 0x00 },
                                                  { 0x1F, 0x3E, 0x7C, 0xF8, 0x00, 0x00, 0x00, 0x00 },
                                                  { 0x3F, 0x7E, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                                  { 0x7F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                                  { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
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

    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_buffer_get_mono10_to_14_packed_image(
        LV_ErrorClusterPtr_t error_cluster_ptr,
        LV_EDVRReferencePtr_t edvr_ref_ptr,
        uint8_t n_bits,
        LV_2DArrayHandle_t<uint16_t> array_handle)
    {
        try
        {
            lv_buffer buffer(edvr_ref_ptr);

            array_handle.size_to_fit({buffer.height(), buffer.width()});

            int32_t row = 0;
            int32_t col = 0;

            auto lsb_source = buffer.begin();

            uint8_t start_bit_offset = 0;

            while(row < buffer.height()){

                array_handle[{row, col}] = 0; // zero out value to allow for bitwise OR accumulation of values
                uint8_t bits = 0;

                while(true){
                    // determine what the offset is for this byte, the number of bits and lookup the mask value
                    auto b = bits == 0? start_bit_offset : 0;
                    auto n = std::min(n_bits - bits, 8 - b);
                    uint8_t mask = mono_packed_masks_lookup[n-1][b];

                    // get the byte, mask and shift so the bits are in the correct location
                    // cast to a 16-bit value and shift to the correct location in the 16-bit value
                    // OR those bits with the previous value in the output array
                    array_handle[{row, col}] |= (static_cast<uint16_t>(*lsb_source & mask) >> b) << bits;

                    // compute the number of bits handled so far
                    bits += n;

                    // check if all done?
                    if(bits == n_bits){
                        break;
                    }

                    // advance to the next input byte
                    lsb_source++;
                }

                // determine the next start location
                start_bit_offset = (start_bit_offset  + n_bits) % 8;

                // handle advancing the input if the current byte has been fully consumed
                if(start_bit_offset == 0){
                    lsb_source++;
                }

                // handle the 2D output array indexing
                col++;
                if(col == buffer.width()){
                    row++;
                    col = 0;
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
}