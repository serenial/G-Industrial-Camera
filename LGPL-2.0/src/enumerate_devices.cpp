#include <arv.h>
#include <g_industrial_cam_export.h>

extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT void enumerate(int* n_devices)
    {
        arv_update_device_list ();
        *n_devices = arv_get_n_devices ();
    }
}