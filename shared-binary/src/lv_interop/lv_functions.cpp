#include <algorithm>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <string>

#include "g_industrial_cam/lv_interop/lv_functions.hpp"
#include "g_industrial_cam/lv_interop/lv_array_1d.hpp"
#include "g_industrial_cam/lv_interop/lv_str.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam_export.h"

using namespace g_industrial_cam;
using namespace lv_interop;

namespace
{
    static std::string lv_runtime_path_windows;
    // static function pointers
    static LV_EDVRGetCurrentContextFnPtr_t EDVR_GetCurrentContextImp = nullptr;
    static LV_EDVRCreateReferenceFnPtr_t EDVR_CreateReferenceImp = nullptr;
    static LV_EDVRAddRefWithContextFnPtr_t EDVR_AddRefWithContextImp = nullptr;
    static LV_EDVRReleaseRefWithContextFnPtr_t EDVR_ReleaseRefWithContextImp = nullptr;
    static LV_DSDisposeHandleFnPtr_t DSDisposeHandleImp = nullptr;
    static LV_DSCheckHandlePtr_t DSCheckHandleImp = nullptr;
    static LV_DSNewHClrPtr_t DSNewHClrImp = nullptr;
    static LV_DSSetHSzClrPtr_t DSSetHSzClrImp = nullptr;
    static LV_DSGetHandleSizePtr_t DSGetHandleSizeImp = nullptr;
    static LV_PostLVUserEventPtr_t PostLVUserEventImp = nullptr;
}

// call this function when the .so is loaded on linux
#if defined(__GNUC__) && !defined(_WIN32)
__attribute__((constructor))
#endif
void g_industrial_cam::on_shared_library_load()
{
    // Import functions from LabVIEW IDE or LabVIEW Runtime
#ifdef _WIN32

            HMODULE module = nullptr;

            if(lv_runtime_path_windows.empty()){
                // try loading from the normal runtimes
                static const char * const runtimes [] = {"LabVIEW.exe","lvffrt.dll","lvrt.dll"};

                // work through the list of runtimes
                for(const auto & runtime : runtimes){
                    module = GetModuleHandle(runtime);
                    if(module) break;
                }
    
                if (!module)
                {
                    return;
                }
            }
            else{
                module = GetModuleHandle(lv_runtime_path_windows.c_str());
                if(!module){
                    // clear the path
                    lv_runtime_path_windows = "";
                    return;
                }
            }

            EDVR_GetCurrentContextImp = reinterpret_cast<LV_EDVRGetCurrentContextFnPtr_t>(GetProcAddress(module, "EDVR_GetCurrentContext"));
            EDVR_CreateReferenceImp = reinterpret_cast<LV_EDVRCreateReferenceFnPtr_t>(GetProcAddress(module, "EDVR_CreateReference"));
            EDVR_AddRefWithContextImp = reinterpret_cast<LV_EDVRAddRefWithContextFnPtr_t>(GetProcAddress(module, "EDVR_AddRefWithContext"));
            EDVR_ReleaseRefWithContextImp = reinterpret_cast<LV_EDVRReleaseRefWithContextFnPtr_t>(GetProcAddress(module, "EDVR_ReleaseRefWithContext"));
            DSDisposeHandleImp = reinterpret_cast<LV_DSDisposeHandleFnPtr_t>(GetProcAddress(module, "DSDisposeHandle"));
            DSCheckHandleImp = reinterpret_cast<LV_DSCheckHandlePtr_t>(GetProcAddress(module, "DSCheckHandle"));
            DSNewHClrImp = reinterpret_cast<LV_DSNewHClrPtr_t>(GetProcAddress(module, "DSNewHClr"));
            DSSetHSzClrImp = reinterpret_cast<LV_DSSetHSzClrPtr_t>(GetProcAddress(module, "DSSetHSzClr"));
            DSGetHandleSizeImp = reinterpret_cast<LV_DSGetHandleSizePtr_t>(GetProcAddress(module, "DSGetHandleSize"));
            PostLVUserEventImp = reinterpret_cast<LV_PostLVUserEventPtr_t>(GetProcAddress(module, "PostLVUserEvent"));
#else
            auto module = dlopen(nullptr, RTLD_LAZY);

            if (!module)
            {
                return;
            }

            EDVR_GetCurrentContextImp = reinterpret_cast<LV_EDVRGetCurrentContextFnPtr_t>(dlsym(module, "EDVR_GetCurrentContext"));
            EDVR_CreateReferenceImp = reinterpret_cast<LV_EDVRCreateReferenceFnPtr_t>(dlsym(module, "EDVR_CreateReference"));
            EDVR_AddRefWithContextImp = reinterpret_cast<LV_EDVRAddRefWithContextFnPtr_t>(dlsym(module, "EDVR_AddRefWithContext"));
            EDVR_ReleaseRefWithContextImp = reinterpret_cast<LV_EDVRReleaseRefWithContextFnPtr_t>(dlsym(module, "EDVR_ReleaseRefWithContext"));
            DSDisposeHandleImp = reinterpret_cast<LV_DSDisposeHandleFnPtr_t>(dlsym(module, "DSDisposeHandle"));
            DSCheckHandleImp = reinterpret_cast<LV_DSCheckHandlePtr_t>(dlsym(module, "DSCheckHandle"));
            DSNewHClrImp = reinterpret_cast<LV_DSNewHClrPtr_t>(dlsym(module, "DSNewHClr"));
            DSSetHSzClrImp = reinterpret_cast<LV_DSSetHSzClrPtr_t>(dlsym(module, "DSSetHSzClr"));
            DSGetHandleSizeImp = reinterpret_cast<LV_DSGetHandleSizePtr_t>(dlsym(module, "DSGetHandleSize"));
            PostLVUserEventImp = reinterpret_cast<LV_PostLVUserEventPtr_t>(dlsym(module, "PostLVUserEvent"));
#endif
}

#if defined(__GNUC__) && !defined(_WIN32)
__attribute__((destructor))
#endif
void g_industrial_cam::on_shared_library_unload()
{
    // nothing to cleanup
}

#if defined(_WIN32)
#if defined(_MSC_VER)
#pragma warning(push, 3)
#endif
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        on_shared_library_load();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        on_shared_library_unload();
        break;
    }
    return true;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif

LV_MgErr_t lv_interop::EDVR_GetCurrentContext(LV_Ptr_t<LV_EDVRContext_t> ctx_ptr)
{
    return EDVR_GetCurrentContextImp? EDVR_GetCurrentContextImp(ctx_ptr): LV_ERR_bogusError;
}
LV_MgErr_t lv_interop::EDVR_CreateReference(LV_EDVRReferencePtr_t edvr_ref_ptr, LV_EDVRDataHandle_t hndl)
{
    return EDVR_CreateReferenceImp? EDVR_CreateReferenceImp(edvr_ref_ptr, hndl): LV_ERR_bogusError;
}
LV_MgErr_t lv_interop::EDVR_AddRefWithContext(LV_EDVRReference_t edvr_ref_ptr, LV_EDVRContext_t ctx, LV_EDVRDataHandle_t hndl)
{
    return EDVR_AddRefWithContextImp? EDVR_AddRefWithContextImp(edvr_ref_ptr, ctx, hndl): LV_ERR_bogusError;
}
LV_MgErr_t lv_interop::EDVR_ReleaseRefWithContext(LV_EDVRReference_t edvr_ref_ptr, LV_EDVRContext_t ctx)
{
    return EDVR_ReleaseRefWithContextImp? EDVR_ReleaseRefWithContextImp(edvr_ref_ptr, ctx): LV_ERR_bogusError;
}
LV_MgErr_t lv_interop::DSDisposeHandle(LV_UHandle_t hndl)
{
    return DSDisposeHandleImp? DSDisposeHandleImp(hndl): LV_ERR_bogusError;
}
LV_MgErr_t lv_interop::DSCheckHandle(LV_UHandle_t hndl)
{
    return DSCheckHandleImp? DSCheckHandleImp(hndl): LV_ERR_bogusError;
}
LV_UHandle_t lv_interop::DSNewHClr(size_t size)
{
    return DSNewHClrImp? DSNewHClrImp(size): nullptr;
}
LV_MgErr_t lv_interop::DSSetHSzClr(LV_UHandle_t hndl, size_t size)
{
    return DSSetHSzClrImp? DSSetHSzClrImp(hndl, size): LV_ERR_bogusError;
}
size_t lv_interop::DSGetHandleSize(LV_UHandle_t hndl)
{
    return DSGetHandleSizeImp? DSGetHandleSizeImp(hndl): LV_ERR_bogusError;
}

LV_MgErr_t lv_interop::PostLVUserEvent(LV_UserEventRef_t ref, void* data){
    return PostLVUserEventImp ? PostLVUserEventImp(ref, data) : LV_ERR_bogusError;
}


LV_MgErr_t lv_interop::get_edvr_data_handle_with_context(LV_EDVRReference_t edvr_ref, LV_EDVRContext_t *cntx_ptr, LV_EDVRDataHandle_t data_handle)
{

    if (!edvr_ref || !cntx_ptr || !data_handle)
    {
        return LV_ERR_mgArgErr;
    }

    auto err = EDVR_GetCurrentContext(cntx_ptr);

    if (err)
    {
        return err;
    }

    if (cntx_ptr == 0)
    {
        return LV_ERR_bogusError;
    }

    err = EDVR_AddRefWithContext(edvr_ref, *cntx_ptr, data_handle);

    return err;
}

#ifdef _WIN32
extern "C"
{
    G_INDUSTRIAL_CAM_EXPORT LV_MgErr_t g_industrial_cam_specify_lv_runtime_windows(const char * const path)
    {
        // set the path
        lv_runtime_path_windows = path;
        // try and load the library
        on_shared_library_load();
        // this will clear the lv_runtime_path_windows string if it fails
        return lv_runtime_path_windows.empty()? LV_ERR_bogusError : LV_ERR_noError;
    }
}
#endif