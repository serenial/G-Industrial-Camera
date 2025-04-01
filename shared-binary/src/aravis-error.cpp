#include "g_industrial_cam/aravis-error.hpp"

using namespace g_industrial_cam;

aravis_error::aravis_error(): m_err(nullptr){
    // nothint to init
}

aravis_error::~aravis_error(){
    if(m_err){
        g_error_free(m_err);
    }
    m_err = nullptr;
}

aravis_error::operator _GError **(){
    return &m_err;
}

std::string aravis_error::message() const{
    if(m_err){
        return std::string(m_err->message);
    }
    return std::string();
}

int32_t aravis_error::code() const{
    if(m_err){
        return m_err->code;
    }
    return 0;
}

void aravis_error::check_error(const aravis_error& e){
    if(e.m_err){
        throw aravis_error_exception(e);
    }
}

aravis_error_exception::aravis_error_exception(const aravis_error& e) : 
    std::system_error(std::error_code(e.code(),std::generic_category()), e.message())
{
    // init done
}