#pragma once

#include <utility>
#include <mutex>
#include <condition_variable>

#include <arv.h>

#include "./lv_types.hpp"

namespace g_industrial_cam
{
    using namespace lv_interop;

    class lv_buffer
    {
    public:
        lv_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr, ArvBuffer *buf);
        lv_buffer(LV_EDVRReferencePtr_t edvr_ref_ptr);
        ~lv_buffer();
        bool is_valid() const;
        bool has_status_success() const;
        const uint8_t *begin() const;
        const uint8_t *end() const;
        const size_t size() const;
        const uint16_t width() const;
        const uint16_t height() const;
        void upgrade_to_mapped();
        void downgrade_from_mapped();
        void reallocate(size_t required_size);

    private:
        struct buffer_persistant_data_t
        {
            enum lock_states
            {
                NONE,
                LABVIEW,
                CPP,
                CPP_MAPPED
            };
            ArvBuffer *m_arv_buffer_ptr;
            lock_states m_locked;
            std::mutex m_mtx;
            std::condition_variable m_cv;
            ~buffer_persistant_data_t();
            // locking and unlocking utility functions
            static void lock(buffer_persistant_data_t *, lock_states);
            static void unlock(buffer_persistant_data_t *, lock_states);
        };

        // private functions required for initialization
        LV_EDVRContext_t get_ctx();
        LV_EDVRDataPtr_t create_new_edvr_data_ptr();
        LV_EDVRDataPtr_t get_edvr_data_ptr();
        buffer_persistant_data_t *get_metadata();
        std::pair<const uint8_t *, const size_t> buffer_image_data() const;

        // private properties - the order here is important for the initialization step
        const LV_EDVRReferencePtr_t edvr_ref_ptr;
        const LV_EDVRContext_t ctx;
        const LV_EDVRDataPtr_t edvr_data_ptr;
        buffer_persistant_data_t *const data;

        static LV_MgErr_t on_labview_lock(LV_EDVRDataPtr_t ptr);
        static LV_MgErr_t on_labview_unlock(LV_EDVRDataPtr_t ptr);
        static void on_labview_delete(LV_EDVRDataPtr_t ptr);
    };

}