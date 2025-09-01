#pragma once

#include <exception>
#include <stdexcept>
#include "./lv_types.hpp"
#include "./lv_str.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

#include "./set_packing.hpp"
namespace g_industrial_cam
{
    namespace lv_interop
    {
        // LabVIEW Error Cluster type
        class LV_ErrorClusterPtr_t
        {
        public:
            LV_ErrorClusterPtr_t() = delete;
            void copy_from_exception(std::exception_ptr ex, const char *caller_name);

        private:
            struct LV_Error_t
            {
                LV_Error_t() = delete;
                LV_Boolean_t status;
                LV_MgErr_t code;
                LV_StringHandle_t source;
            };
            LV_Ptr_t<LV_Error_t> m_err;
        };
    }
}
#include "./reset_packing.hpp"