#include <cstring>
#include <string>
#include <sstream>

#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_edvr_managed_object.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

void LV_ErrorClusterPtr_t::copy_from_exception(const std::exception_ptr ex, const char * caller_name){

    std::stringstream ss;

    ss << caller_name << "<ERR>";

    try
    {
        if (ex)
        {
            std::rethrow_exception(ex);
        }
    }
    catch (LV_MemoryManagerException const&e)
    {
        ss << e.what();
        m_err->code = e.err;
    }
    catch (LV_EDVRInvalidException const&e)
    {
        ss << e.what();
        m_err->code = 1556;
    }
    catch (std::system_error const&e){
        ss << e.what();
        m_err->code = e.code().value();
    }
    catch (std::exception const&e)
    {
        ss << e.what();
        m_err->code = LV_ERR_bogusError;
    }
    catch (...)
    {
        ss << "An undefined exception occured.";
        m_err->code = LV_ERR_bogusError;
    }

    m_err->status = m_err->code != 0;

    m_err->source.copy_from(ss.str());
}