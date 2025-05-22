#include <stdexcept>
#include <functional>

#include "g_industrial_cam/lv_interop/lv_functions.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

bool lv_buffer::is_valid() const
{
    return data->m_arv_buffer_ptr != nullptr;
}

bool lv_buffer::has_status_success() const
{
    if(is_valid()){
        return arv_buffer_get_status(data->m_arv_buffer_ptr) == ARV_BUFFER_STATUS_SUCCESS;
    }

    return false;
}

const uint8_t *lv_buffer::begin() const
{
    if(is_valid()){
        return buffer_image_data().first;
    }
    return nullptr;
}

const uint8_t *lv_buffer::end() const
{
    if(is_valid()){
    auto image_data = buffer_image_data();
    return image_data.first + image_data.second;
    }
    return nullptr;
}

const size_t lv_buffer::size() const
{
    if(is_valid()){
    return buffer_image_data().second;
    }

    return 0;
}

std::pair<const uint8_t *, const size_t> lv_buffer::buffer_image_data() const
{
    if(is_valid()){
    size_t size;
    auto begin = static_cast<const uint8_t *>(arv_buffer_get_image_data(data->m_arv_buffer_ptr, &size));
    return std::make_pair(begin, size);
    }

    return std::pair<const uint8_t *, const size_t>(nullptr, 0);
}

const uint16_t lv_buffer::width() const
{
    if(is_valid()){
    return arv_buffer_get_image_width(data->m_arv_buffer_ptr);
    }
    return 0;
}

const uint16_t lv_buffer::height() const
{
    if(is_valid()){
    return arv_buffer_get_image_height(data->m_arv_buffer_ptr);
    }
    return 0;
}

lv_buffer::lv_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr, ArvBuffer *buf) : edvr_ref_ptr(edvr_ref_ptr),
                                                                           ctx(0),
                                                                           edvr_data_ptr(create_new_edvr_data_ptr()),
                                                                           data(get_metadata())

{
    data->m_arv_buffer_ptr = buf;
    buffer_persistant_data_t::lock(data, buffer_persistant_data_t::lock_states::CPP);
}

lv_buffer::lv_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr) : edvr_ref_ptr(edvr_ref_ptr), ctx(get_ctx()),
                                                           edvr_data_ptr(get_edvr_data_ptr()),
                                                           data(get_metadata())
{
    buffer_persistant_data_t::lock(data, buffer_persistant_data_t::lock_states::CPP);
}

lv_buffer::~lv_buffer()
{
    // ensure EDVR sub_array data is correct
    // set the n_dims
    edvr_data_ptr->n_dims = 1;
    // set the subArray data pointer to the buffers data
    edvr_data_ptr->sub_array.data_ptr = reinterpret_cast<uintptr_t *>(const_cast<uint8_t *>(begin()));
    // set subArray dimension_specifier
    // strides/steps are in bytes, size is number of elements so we can directly use values
    // datatype is a single byte so stride is just 1.
    edvr_data_ptr->sub_array.dimension_specifier[0] = {size(), ptrdiff_t(1)};

    // unlock
    buffer_persistant_data_t::unlock(data, buffer_persistant_data_t::lock_states::CPP);

    // release ref if originally added
    if (ctx)
    {
        EDVR_ReleaseRefWithContext(*edvr_ref_ptr, ctx);
    }
}

LV_EDVRContext_t lv_buffer::get_ctx()
{
    LV_EDVRContext_t ctx;
    auto err = EDVR_GetCurrentContext(&ctx);
    if (err)
    {
        throw std::runtime_error("Failed to obtain the application-context.");
    }
    return ctx;
}

LV_EDVRDataPtr_t lv_buffer::create_new_edvr_data_ptr()
{
    // initialize EDVR reference and get new EDVR data_ptr

    LV_EDVRDataPtr_t data_ptr = nullptr;
    auto err = EDVR_CreateReference(edvr_ref_ptr, &data_ptr);
    if (err)
    {
        throw std::runtime_error("Unable to create a data-reference to associate with the supplied EDVR Refnum.");
    }

    // set the metadata_ptr of the new edvr data pointer
    data_ptr->metadata_ptr = reinterpret_cast<uintptr_t>(new buffer_persistant_data_t());

    // set the callback functions
    data_ptr->lock_callback_fn_ptr = &lv_buffer::on_labview_lock;
    data_ptr->unlock_callback_fn_ptr = &lv_buffer::on_labview_unlock;
    data_ptr->delete_callback_fn_ptr = &lv_buffer::on_labview_delete;

    return data_ptr;
}

LV_EDVRDataPtr_t lv_buffer::get_edvr_data_ptr()
{
    LV_EDVRDataPtr_t data_ptr = nullptr;
    auto err = EDVR_AddRefWithContext(*edvr_ref_ptr, ctx, &data_ptr);
    if (err)
    {
        throw std::runtime_error("Unable to dereference the supplied EDVR Refnum to valid data in this application-context.");
    }
    return data_ptr;
}

lv_buffer::buffer_persistant_data_t *lv_buffer::get_metadata()
{
    return reinterpret_cast<buffer_persistant_data_t *>(edvr_data_ptr->metadata_ptr);
}

void lv_buffer::buffer_persistant_data_t::lock(lv_buffer::buffer_persistant_data_t *d, lv_buffer::buffer_persistant_data_t::lock_states transition_to)
{
    // obtain the mutex
    std::unique_lock lk(d->m_mtx);
    // wait for the locked flag to be NONE
    // this will lead to deadlocks if CPP or CPP_MAPPED but that is probably desired behaviour
    d->m_cv.wait(lk, [&]
                 { return d->m_locked == NONE; });
    d->m_locked = transition_to;
    lk.unlock();
    d->m_cv.notify_all();
}

void lv_buffer::buffer_persistant_data_t::unlock(lv_buffer::buffer_persistant_data_t *d, lv_buffer::buffer_persistant_data_t::lock_states transition_from)
{
    {
        // obtain the mutex
        std::lock_guard lk(d->m_mtx);
        if (d->m_locked == transition_from)
        {
            d->m_locked = NONE;
        };
    }
    // scoped-unlock and notify
    d->m_cv.notify_all();
}

lv_buffer::buffer_persistant_data_t::~buffer_persistant_data_t()
{
    g_object_unref(m_arv_buffer_ptr);
    m_arv_buffer_ptr = nullptr;
}

void lv_buffer::upgrade_to_mapped()
{
    {
        // obtain the mutex
        std::lock_guard lk(data->m_mtx);
        if (data->m_locked == buffer_persistant_data_t::lock_states::CPP)
        {
            data->m_locked = buffer_persistant_data_t::lock_states::CPP_MAPPED;
        };
    }
    // scoped-unlock and notify
    data->m_cv.notify_all();
}

void lv_buffer::downgrade_from_mapped()
{
    {
        // obtain the mutex
        std::lock_guard lk(data->m_mtx);
        if (data->m_locked == buffer_persistant_data_t::lock_states::CPP_MAPPED)
        {
            data->m_locked = buffer_persistant_data_t::lock_states::CPP;
        };
    }
    // scoped-unlock and notify
    data->m_cv.notify_all();
}

LV_MgErr_t lv_buffer::on_labview_lock(LV_EDVRDataPtr_t ptr)
{
    // LabVIEW is trying to obtain the data referenced by the EDVR
    try
    {
        auto data = reinterpret_cast<buffer_persistant_data_t *>(ptr->metadata_ptr);
        buffer_persistant_data_t::lock(data, buffer_persistant_data_t::lock_states::LABVIEW);
    }
    catch (...)
    {
        return LV_ERR_bogusError;
    }

    return LV_ERR_noError;
}

LV_MgErr_t lv_buffer::on_labview_unlock(LV_EDVRDataPtr_t ptr)
{
    try
    {
        auto data = reinterpret_cast<buffer_persistant_data_t *>(ptr->metadata_ptr);
        buffer_persistant_data_t::unlock(data, buffer_persistant_data_t::lock_states::LABVIEW);
    }
    catch (...)
    {
        return LV_ERR_bogusError;
    }
    return LV_ERR_noError;
}

void lv_buffer::on_labview_delete(LV_EDVRDataPtr_t ptr)
{
    auto data = reinterpret_cast<buffer_persistant_data_t *>(ptr->metadata_ptr);
    // obtain the mutex
    std::unique_lock lk(data->m_mtx);
    // wait for the locked flag to be cleared
    // note - DVR Delete calls lock_callback_fn first so only check we aren't locked from CPP side
    data->m_cv.wait(lk, [&]
                    { return data->m_locked == buffer_persistant_data_t::lock_states::NONE || data->m_locked == buffer_persistant_data_t::lock_states::LABVIEW; });
    // free the lock
    lk.unlock();
    // delete data
    delete data;
    ptr->metadata_ptr = reinterpret_cast<uintptr_t>(nullptr);
}

void lv_buffer::reallocate(size_t required_size){
    if(size() != required_size){
        // unref the current buffer
        g_object_unref(data->m_arv_buffer_ptr);
        data->m_arv_buffer_ptr = arv_buffer_new_allocate(required_size);
    }
}

lv_buffer::operator ArvBuffer**() const{
    return &data->m_arv_buffer_ptr;
}