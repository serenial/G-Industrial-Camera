#pragma once

#include <vector>
#include <array>
#include <functional>
#include <cstring>
#include <numeric>
#include <exception>
#include <algorithm>
#include <stdexcept>
#include <span>

#include "./lv_types.hpp"
#include "./lv_functions.hpp"

#include "./set_packing.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        template <class T>
        class LV_1DArrayHandle_t
        {

        public:
            LV_1DArrayHandle_t() = delete;
            
            bool empty() const
            {
                return size() == 0;
            }

            T &operator[](int i)
            {
                if (i >= 0 && i < size())
                {
                    return (*m_handle)->data[i];
                }
                throw std::out_of_range("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            T const &operator[](int i) const
            {
                if (i >=0 && i < size())
                {
                    return (*m_handle)->data[i];
                }
                throw std::out_of_range("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            std::size_t size() const
            {
                return is_valid_handle() ? (*m_handle)->dims[0] : 0;
            }

            T *begin() const noexcept
            {
                return is_valid_handle() ? (*m_handle)->data_ptr() : nullptr;
            }

            T *end() const noexcept
            {
                return is_valid_handle() ? (*m_handle)->data_ptr() + size() : nullptr;
            }

            void size_to_fit(size_t n_elements)
            {
                // allocated memory will be zero'd out by DSNewHClr and DSSetHszClr as this is the better
                // option when we might be allocating other array handles in the new space

                auto bytes =  LV_Array_t<1, T>::data_member_offset_bytes() + sizeof(T) * n_elements;

                if (!is_valid_handle())
                {
                    m_handle = reinterpret_cast<LV_Handle_t<LV_Array_t<1, T>>>(DSNewHClr(bytes));

                    if(!m_handle){
                        throw LV_MemoryManagerException(LV_ERR_mFullErr);
                    }
                    (*m_handle)->dims[0] = n_elements;
                    return;
                }

                auto uhandle = reinterpret_cast<LV_UHandle_t>(m_handle);

                auto current_bytes = DSGetHandleSize(uhandle);

                if (bytes > current_bytes)
                {
                    auto err = DSSetHSzClr(uhandle, bytes);
                    if (err)
                    {
                        throw LV_MemoryManagerException(err);
                    }
                }

                (*m_handle)->dims[0] = n_elements;
            }

            // copy from for non fundamental types that requires each element to be copied in turn
            template <class ElementType, typename CopyFunction = std::function<void(const ElementType &, T *)>>
            void copy_element_by_element_from(std::vector<ElementType> vector, CopyFunction copy_fn = [](const auto &from, auto to)
                                                                               { *to = from; })
            {
                size_to_fit(vector.size());

                auto vector_it = vector.begin();
                auto array_it = begin();

                for (; vector_it != vector.end() && array_it != end(); ++vector_it, ++array_it)
                {
                    copy_fn(*vector_it, array_it);
                }
            }

            // copy from for contiguous fundamental types which can be copied in a block
            template <class ElementType>
            void copy_memory_from(const std::vector<ElementType>& vector)
            {
                // memcpy only works with fundamental types
                static_assert(std::is_trivially_copyable_v<ElementType> == true, "Default copying is only compatible for fundamental value types. Specify the copy function [](auto from, auto to){ /* do conversion */}");
                size_to_fit(vector.size());
                std::memcpy(begin(), vector.data(), vector.size() * sizeof(T));
            }

            // copy from for std::span
            template <class ElementType>
            void copy_memory_from(const std::span<ElementType>& span)
            {
                size_to_fit(span.size());
                // copy block
                std::memcpy(begin(), span.begin(), span.size());
                return;
            }

            template <typename T_vec>
            std::vector<T_vec> to_vector() const
            {

                std::vector<T_vec> output_vector;

                output_vector.reserve(size());

                for (const auto &e : *this)
                {
                    output_vector.push_back(e);
                }

                return output_vector;
            }

            template <typename T_vec>
            std::vector<T_vec> as_vector() const
            {

                std::vector<T_vec> output_vector;

                output_vector.reserve(size());
                // note the lack of const
                for (auto &e : *this)
                {
                    output_vector.push_back(e);
                }

                return output_vector;
            }
        private:
            LV_Handle_t<LV_Array_t<1, T>> m_handle;

            bool is_valid_handle() const{
                return DSCheckHandle(reinterpret_cast<LV_UHandle_t>(m_handle)) == LV_ERR_noError;
            }
        };
    }
}

#include "./reset_packing.hpp"
