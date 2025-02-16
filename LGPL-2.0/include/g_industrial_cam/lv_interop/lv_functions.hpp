#pragma once

#ifdef _WIN32
// include windows.h
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <initializer_list>
#include "./lv_types.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        LV_MgErr_t EDVR_GetCurrentContext(LV_Ptr_t<LV_EDVRContext_t>);
        LV_MgErr_t EDVR_CreateReference(LV_EDVRReferencePtr_t, LV_EDVRDataHandle_t);
        LV_MgErr_t EDVR_AddRefWithContext(LV_EDVRReference_t, LV_EDVRContext_t, LV_EDVRDataHandle_t);
        LV_MgErr_t EDVR_ReleaseRefWithContext(LV_EDVRReference_t, LV_EDVRContext_t);
        LV_MgErr_t NumericArrayResize(int32_t type, int32_t n_dims, LV_UHandlePtr_t, size_t size);
        LV_MgErr_t DSDisposeHandle(LV_UHandle_t);
        LV_MgErr_t DSCheckHandle(LV_UHandle_t);
        LV_UHandle_t DSNewHandle(size_t);
        LV_MgErr_t DSSetHandleSize(LV_UHandle_t, size_t);
        size_t DSGetHandleSize(LV_UHandle_t);

        // MgErr EDVR_GetCurrentContext(ExternalDataValueReferenceContext* pContext);
        using LV_EDVRGetCurrentContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_Ptr_t<LV_EDVRContext_t>)>;

        // MgErr EDVR_CreateReference(ExternalDataValueReference* pReference, ExternalDataValueReferenceData** ppReferenceData);
        using LV_EDVRCreateReferenceFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReferencePtr_t, LV_EDVRDataHandle_t)>;

        // MgErr EDVR_AddRefWithContext(ExternalDataValueReference reference, ExternalDataValueReferenceContext context, ExternalDataValueReferenceData** ppReferenceData);
        using LV_EDVRAddRefWithContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReference_t, LV_EDVRContext_t, LV_EDVRDataHandle_t)>;

        // MgErr EDVR_ReleaseRefWithContext(ExternalDataValueReference reference, ExternalDataValueReferenceContext context);
        using LV_EDVRReleaseRefWithContextFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_EDVRReference_t, LV_EDVRContext_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/numericarrayresize.html
        // MgErr NumericArrayResize (int32 typeCode, int32 numDims, Uhandle *dataHP, size_t totalNewSize)
        using LV_NumericArrayResizeFnPtr_t = std::add_pointer_t<LV_MgErr_t(int32_t, int32_t, LV_UHandlePtr_t, size_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/dsdisposehandle.html
        // MgErr DSDisposeHandle(h)
        using LV_DSDisposeHandleFnPtr_t = std::add_pointer_t<LV_MgErr_t(LV_UHandle_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/dscheckhandle.html
        // MgErr DSCheckHandle(h);
        using LV_DSCheckHandlePtr_t = std::add_pointer_t<LV_MgErr_t(LV_UHandle_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/dsnewhandle.html
        // UHandle DSNewHandle(size);
        using LV_DSNewHandlePtr_t = std::add_pointer_t<LV_UHandle_t(size_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/dssethandlesize.html
        // MgErr DSSetHandleSize(h, size);
        using LV_DSSetHandleSizePtr_t = std::add_pointer_t<LV_MgErr_t(LV_UHandle_t, size_t)>;

        // https://www.ni.com/docs/en-US/bundle/labview-api-ref/page/properties-and-methods/lv-manager/dsgethandlesize.html
        // size_t DSGetHandleSize(h);
        using LV_DSGetHandleSizePtr_t = std::add_pointer_t<size_t(LV_UHandle_t)>;

        using LV_NumericType_t = int32_t;
        const LV_NumericType_t LV_NumericType_I8 = 0x01;
        const LV_NumericType_t LV_NumericType_I16 = 0x02;
        const LV_NumericType_t LV_NumericType_I32 = 0x03;
        const LV_NumericType_t LV_NumericType_I64 = 0x04;
        const LV_NumericType_t LV_NumericType_U8 = 0x05;
        const LV_NumericType_t LV_NumericType_U16 = 0x06;
        const LV_NumericType_t LV_NumericType_U32 = 0x07;
        const LV_NumericType_t LV_NumericType_U64 = 0x08;
        const LV_NumericType_t LV_NumericType_SGL = 0x09;
        const LV_NumericType_t LV_NumericType_DBL = 0x0A;
        const LV_NumericType_t LV_NumericType_EXT = 0x0B;
        const LV_NumericType_t LV_NumericType_CSG = 0x0C;
        const LV_NumericType_t LV_NumericType_CDB = 0x0D;
        const LV_NumericType_t LV_NumericType_CXT = 0x0E;

#ifdef g_industrial_cam_BYTE_PACKING_4
        const LV_NumericType_t LV_NumericType_PtrSized = LV_NumericType_U32;
#else
        const LV_NumericType_t LV_NumericType_PtrSized = LV_NumericType_U64;
#endif

        LV_MgErr_t get_edvr_data_handle_with_context(LV_EDVRReference_t, LV_EDVRContext_t *, LV_EDVRDataHandle_t);
        void throw_if_edvr_ref_pointers_not_unique(std::initializer_list<LV_EDVRReferencePtr_t>);

        // custom exceptions
        struct LV_MemoryManagerException : public std::exception
        {
            const LV_MgErr_t err;
            LV_MemoryManagerException(LV_MgErr_t err) : err(err){};
            const char *what() const throw()
            {
                switch (err)
                {
                case LV_ERR_mFullErr:
                    return "An \"mFullErr\" LabVIEW Memory Manager error has occurred.";
                default:
                    return "An unspecified LabVIEW Memory Manager error has occurred.";
                }
            };
        };
    }

    void on_shared_library_load();
    void on_shared_library_unload();
}