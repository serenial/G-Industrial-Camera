// an object that works similar to image::Image but
// only uses the EDVR for lifetime managment and never
// sets the sub-array

#pragma once

#include <memory>
#include <mutex>
#include <functional>
#include <condition_variable>

#include "./lv_types.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {

        struct LV_EDVRInvalidException : public std::exception
        {
            const char *what() const throw()
            {
                return "Invalid Reference - ensure the object referenced has been created and is still valid.";
            };
        };
        
        // an which can interact with EDVR references and the persistant data that they refer to
        // similar to Image but only uses the EDVR for lifetime management and never assigns
        // any memory to the EDVR-sub-array
        template <typename T>
        class EDVRManagedObject
        {
        private:

            enum lock_states
            {
                NONE,
                LABVIEW,
                CPP
            };

            struct persistant_data
            {
                T* object;
                lock_states locked;
                std::mutex m;
                std::condition_variable cv;
                const std::function<LV_MgErr_t(T*,LV_EDVRDataPtr_t)> on_lock, on_unlock;
                persistant_data(std::function<LV_MgErr_t(T*, LV_EDVRDataPtr_t)> on_lock, std::function<LV_MgErr_t(T*, LV_EDVRDataPtr_t ptr)> on_unlock): on_lock(on_lock), on_unlock(on_unlock){}
                ~persistant_data(){
                    delete object;
                }
            };

            // locking and unlocking utility functions
            static inline void lock(EDVRManagedObject::persistant_data *d, lock_states transition_to)
            {
                // obtain the mutex
                std::unique_lock lk(d->m);
                // wait for the locked flag to be false
                d->cv.wait(lk, [&]
                           { return (d->locked == NONE); });
                // set the locked flag
                d->locked = transition_to;
                // unlock and notify
                lk.unlock();
                d->cv.notify_one();
            }
            static inline void unlock(EDVRManagedObject::persistant_data *d, lock_states transition_from)
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

            static LV_MgErr_t on_labview_lock(LV_EDVRDataPtr_t ptr)
            {
                // LabVIEW is trying to obtain the data referenced by the EDVR
                try
                {
                    persistant_data *pdp = reinterpret_cast<persistant_data *>(ptr->metadata_ptr);
                    EDVRManagedObject::lock(pdp, lock_states::LABVIEW);
                    return pdp->on_lock(pdp->object, ptr);
                }
                catch (...)
                {
                    return LV_ERR_bogusError;
                }
            
                return LV_ERR_noError;
            }
            
            static LV_MgErr_t on_labview_unlock(LV_EDVRDataPtr_t ptr)
            {
                LV_MgErr_t result = LV_ERR_noError;
                persistant_data *pdp = reinterpret_cast<persistant_data *>(ptr->metadata_ptr);
                try
                {
                      result = pdp->on_unlock(pdp->object, ptr);
                }
                catch (...)
                {
                    result = LV_ERR_bogusError;
                }

                EDVRManagedObject::unlock(pdp, lock_states::LABVIEW);
                return result;
            }
            
            static void on_labview_delete(LV_EDVRDataPtr_t ptr)
            {
                persistant_data *pdp = reinterpret_cast<persistant_data *>(ptr->metadata_ptr);
                // obtain the mutex
                std::unique_lock lk(pdp->m);
                // wait for the locked flag to be cleared
                // note - DVR Delete calls lock_callback_fn first so only check we aren't locked from CPP side
                pdp->cv.wait(lk, [&]
                              { return pdp->locked == lock_states::NONE || pdp->locked == lock_states::LABVIEW; });
                // free the lock
                lk.unlock();
                // delete data
                delete pdp;
                ptr->metadata_ptr = reinterpret_cast<uintptr_t>(nullptr);
            }

            // private functions required for initialization
            LV_EDVRContext_t get_ctx()
            {
                LV_EDVRContext_t ctx;
                auto err = EDVR_GetCurrentContext(&ctx);
                if (err)
                {
                    throw std::runtime_error("Failed to obtain the application-context.");
                }
                return ctx;
            }

            LV_EDVRDataPtr_t create_new_edvr_data_ptr(std::function<LV_MgErr_t(T*,LV_EDVRDataPtr_t)> on_lock, std::function<LV_MgErr_t(T*,LV_EDVRDataPtr_t)> on_unlock)
            {
                // initialize EDVR reference and get new EDVR data_ptr

                LV_EDVRDataPtr_t data_ptr = nullptr;
                auto err = EDVR_CreateReference(edvr_ref_ptr, &data_ptr);
                if (err)
                {
                    throw std::runtime_error("Unable to create a data-reference to associate with the supplied EDVR Refnum.");
                }

                // initialize persistant_data object
                persistant_data *persistant_data_ptr = new persistant_data(on_lock, on_unlock);

                // set the metadata_ptr of the new edvr data pointer
                data_ptr->metadata_ptr = reinterpret_cast<uintptr_t>(persistant_data_ptr);

                // locking and unlocking from labVIEW side
                data_ptr->lock_callback_fn_ptr = &EDVRManagedObject::on_labview_lock;

                data_ptr->unlock_callback_fn_ptr = &EDVRManagedObject::on_labview_unlock;

                data_ptr->delete_callback_fn_ptr = &EDVRManagedObject::on_labview_delete;

                return data_ptr;
            }

            LV_EDVRDataPtr_t get_edvr_data_ptr()
            {
                LV_EDVRDataPtr_t data_ptr = nullptr;
                auto err = EDVR_AddRefWithContext(*edvr_ref_ptr, ctx, &data_ptr);
                if (err)
                {
                    throw LV_EDVRInvalidException();
                }
                return data_ptr;
            }

            persistant_data *get_data()
            {
                return reinterpret_cast<persistant_data *>(edvr_data_ptr->metadata_ptr);
            }

            // private properties - the order here is important for the initialization step
            const LV_EDVRReferencePtr_t edvr_ref_ptr;
            const LV_EDVRContext_t ctx;
            const LV_EDVRDataPtr_t edvr_data_ptr;
            persistant_data *const data;

        public:
            // create from exisiting object
            EDVRManagedObject(LV_EDVRReferencePtr_t edvr_ref_ptr) : edvr_ref_ptr(edvr_ref_ptr), ctx(get_ctx()),
                                                                    edvr_data_ptr(get_edvr_data_ptr()),
                                                                    data(get_data())
            {
                lock(data, lock_states::CPP);
            }

            // initialize and add new object
            EDVRManagedObject(
                LV_EDVRReferencePtr_t edvr_ref_ptr, 
                T* object, 
                std::function<void(T* ,const LV_EDVRDataPtr_t)> edvr_value_set_function = [](auto ptr, auto e_data_ptr)
                                                                                          {
                        // set the sub-array to be a 1-D empty array
                        e_data_ptr->n_dims = 1;
                        e_data_ptr->sub_array.dimension_specifier[0] = {0, 1}; },
                std::function<LV_MgErr_t(T*, LV_EDVRDataPtr_t)> on_lock = [](auto p, auto d){ return LV_ERR_noError;},
                std::function<LV_MgErr_t(T*, LV_EDVRDataPtr_t)> on_unlock = [](auto p, auto d){ return LV_ERR_noError;}
            ) : edvr_ref_ptr(edvr_ref_ptr), ctx(0), edvr_data_ptr(create_new_edvr_data_ptr(on_lock,on_unlock)), data(get_data())
            {
                data->object = object;
                edvr_value_set_function(data->object, edvr_data_ptr);
                data->locked = lock_states::CPP; // exclusive access so can lock manually
            }

            ~EDVRManagedObject()
            {
                unlock(data, lock_states::CPP);
                // release ref if originally added
                if (ctx)
                {
                    EDVR_ReleaseRefWithContext(*edvr_ref_ptr, ctx);
                }
            }

            T* operator->()
            {
                return data->object;
            }

            T* operator->() const
            {
                return data->object;
            }

            T& operator*() { 
                return *data->object; 
            }

            T& operator*() const{ 
                return *data->object; 
            }
        };
    }
}
