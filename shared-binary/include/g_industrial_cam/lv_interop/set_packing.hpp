// set the packing and disable the MSVC warning
// G_INDUSTRIAL_CAM_BYTE_PACKING_4 defined at compile time

#ifdef G_INDUSTRIAL_CAM_BYTE_PACKING_4
#pragma pack(push, 1)
#pragma warning (disable : 4103)
#endif