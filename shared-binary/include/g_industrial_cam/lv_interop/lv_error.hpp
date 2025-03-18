#pragma once

#include <exception>
#include <stdexcept>
#include "./lv_types.hpp"
#include "./lv_str.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {

#include "./set_packing.hpp"
        // LabVIEW Error Cluster type
        class LV_ErrorCluster_t
        {
        public:
            void copy_from_exception(std::exception_ptr ex, const char *caller_name);

        private:
            LV_Boolean_t m_status;
            LV_MgErr_t m_code;
            LV_StringHandle_t m_source;
        };
#include "./reset_packing.hpp"

        using LV_ErrorClusterPtr_t = LV_Ptr_t<LV_ErrorCluster_t>;
    }

}