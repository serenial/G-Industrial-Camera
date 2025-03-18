// set the packing and disable the MSVC warning
// g_industrial_cam_BYTE_PACKING_4 defined at compile time

#ifdef g_industrial_cam_BYTE_PACKING_4
#pragma pack(push, 1)
#pragma warning (disable : 4103)
#endif