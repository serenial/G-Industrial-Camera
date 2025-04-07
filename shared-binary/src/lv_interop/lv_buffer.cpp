#include <stdexcept>
#include <functional>

#include "g_industrial_cam/lv_interop/lv_functions.hpp"
#include "g_industrial_cam/lv_interop/lv_error.hpp"
#include "g_industrial_cam/lv_interop/lv_buffer.hpp"

using namespace g_industrial_cam;
using namespace lv_interop;

bool lv_buffer::is_valid() const
{
    return data->buffer != nullptr;
}

bool lv_buffer::has_status_success() const
{
    return arv_buffer_get_status(data->buffer) == ARV_BUFFER_STATUS_SUCCESS;
}

const uint8_t *lv_buffer::begin() const
{
    return buffer_image_data().first;
}

const uint8_t *lv_buffer::end() const
{
    auto image_data = buffer_image_data();
    return image_data.first + image_data.second;
}

const size_t lv_buffer::size() const
{
    return buffer_image_data().second;
}

std::pair<const uint8_t *, const size_t> lv_buffer::buffer_image_data() const
{
    size_t size;
    auto begin = static_cast<const uint8_t *>(arv_buffer_get_image_data(data->buffer, &size));
    return std::make_pair(begin, size);
}

const uint16_t lv_buffer::width() const{
    return arv_buffer_get_image_width(data->buffer);
}

const uint16_t lv_buffer::height() const{
    return arv_buffer_get_image_height(data->buffer);
}

lv_buffer::lv_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr, ArvBuffer * buf) : edvr_ref_ptr(edvr_ref_ptr),
                                                                                      ctx(0),
                                                                                      edvr_data_ptr(create_new_edvr_data_ptr()),
                                                                                      data(get_metadata())

{
    data->buffer = buf;
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
    edvr_data_ptr->sub_array.data_ptr = reinterpret_cast<uintptr_t *>(const_cast<uint8_t*>(begin()));
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
    std::unique_lock lk(d->m);
    // wait for the locked flag to be NONE
    // this will lead to deadlocks if CPP or CPP_MAPPED but that is probably desired behaviour
    d->cv.wait(lk, [&]
               { return d->locked == NONE; });
    d->locked = transition_to;
    lk.unlock();
    d->cv.notify_all();
}

void lv_buffer::buffer_persistant_data_t::unlock(lv_buffer::buffer_persistant_data_t *d, lv_buffer::buffer_persistant_data_t::lock_states transition_from)
{
    {
        // obtain the mutex
        std::lock_guard lk(d->m);
        if (d->locked == transition_from)
        {
            d->locked = NONE;
        };
    }
    // scoped-unlock and notify
    d->cv.notify_all();
}

void lv_buffer::upgrade_to_mapped()
{
    {
        // obtain the mutex
        std::lock_guard lk(data->m);
        if (data->locked == buffer_persistant_data_t::lock_states::CPP)
        {
            data->locked = buffer_persistant_data_t::lock_states::CPP_MAPPED;
        };
    }
    // scoped-unlock and notify
    data->cv.notify_all();
}

void lv_buffer::downgrade_from_mapped()
{
    {
        // obtain the mutex
        std::lock_guard lk(data->m);
        if (data->locked == buffer_persistant_data_t::lock_states::CPP_MAPPED)
        {
            data->locked = buffer_persistant_data_t::lock_states::CPP;
        };
    }
    // scoped-unlock and notify
    data->cv.notify_all();
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
    std::unique_lock lk(data->m);
    // wait for the locked flag to be cleared
    // note - DVR Delete calls lock_callback_fn first so only check we aren't locked from CPP side
    data->cv.wait(lk, [&]
                  { return data->locked == buffer_persistant_data_t::lock_states::NONE || data->locked == buffer_persistant_data_t::lock_states::LABVIEW; });
    // free the lock
    lk.unlock();
    // delete data
    delete data;
    ptr->metadata_ptr = reinterpret_cast<uintptr_t>(nullptr);
}