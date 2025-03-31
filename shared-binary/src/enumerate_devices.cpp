#include <arv.h>

#include <g_industrial_cam/lv_interop/lv_array_1d.hpp>
#include <g_industrial_cam/lv_interop/lv_error.hpp>
#include <g_industrial_cam/lv_interop/lv_str.hpp>

#include <g_industrial_cam_export.h>

using namespace g_industrial_cam;
using namespace lv_interop;

namespace
{
#include <g_industrial_cam/lv_interop/set_packing.hpp>

    struct LV_DeviceEnumeration_t
    {
        LV_StringHandle_t id, protocol, vendor, model, serial, address;
    };

#include <g_industrial_cam/lv_interop/reset_packing.hpp>
}

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t  g_industrial_cam_enumerate_devices(LV_ErrorClusterPtr_t error_cluster_ptr, LV_1DArrayHandle_t<LV_DeviceEnumeration_t> enum_handle)
    {
        try
        {
            arv_update_device_list();
            int n_devices = arv_get_n_devices();

            enum_handle.size_to_fit(n_devices);
            auto enum_element = enum_handle.begin();

            for (int index = 0; index < n_devices; index++)
            {
                enum_element->id.copy_from_utf8(arv_get_device_id(index));
                enum_element->protocol.copy_from_utf8(arv_get_device_protocol(index));
                enum_element->vendor.copy_from_utf8(arv_get_device_vendor(index));
                enum_element->model.copy_from_utf8(arv_get_device_model(index));
                enum_element->serial.copy_from_utf8(arv_get_device_serial_nbr(index));
                enum_element->address.copy_from_utf8(arv_get_device_address(index));
                enum_element++;
            }
        }
        catch (...)
        {
            error_cluster_ptr->copy_from_exception(std::current_exception(), __func__);
        }
        return LV_ERR_noError;
    }
}