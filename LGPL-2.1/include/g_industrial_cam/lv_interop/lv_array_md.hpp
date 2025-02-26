#pragma once

#include <vector>
#include <array>
#include <functional>
#include <cstring>
#include <numeric>
#include <exception>
#include <algorithm>

#include "./lv_types.hpp"
#include "./lv_functions.hpp"

#include "./set_packing.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        template <size_t n_dims, class T>
        class LV_MDArrayHandle_t
        {
        public:
            LV_MDArrayHandle_t() = delete;

            std::array<int32_t, n_dims> extents() const
            {
                std::array<int32_t, n_dims> ex;

                ex.fill(0);

                if (is_valid_handle())
                {
                    for (int i = 0; i < n_dims; i++)
                    {
                        ex[i] = (*m_handle)->dims[i];
                    }
                }
                return ex;
            }

            T &operator[](std::array<int32_t, n_dims> el)
            {
                if (element_is_in_data_range(el))
                {
                    return (*m_handle)->data[get_data_index(el)];
                }
                throw std::invalid_argument("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            T const &operator[](std::array<int32_t, n_dims> el) const
            {
                if (element_is_in_data_range(el))
                {
                    return (*m_handle)->data[get_data_index(el)];
                }
                throw std::invalid_argument("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            T *at(std::array<int32_t, n_dims> el) const noexcept
            {
                return (*m_handle)->data_ptr() + get_data_index(el);
            }

            T *begin() const noexcept
            {
                return (*m_handle)->data_ptr();
            }

            T *end() const noexcept
            {
                return at(extents());
            }

            void size_to_fit(std::array<int32_t, n_dims> n_elements)
            {
                auto required_total_elements = std::accumulate(n_elements.begin(), n_elements.end(), 1, std::multiplies<int32_t>{});
                auto required_bytes = LV_Array_t<n_dims, T>::data_member_offset_bytes() + sizeof(T) * required_total_elements;
                auto uhandle = reinterpret_cast<LV_UHandle_t>(m_handle);

                if (!is_valid_handle())
                {
                    m_handle = reinterpret_cast<LV_Handle_t<LV_Array_t<n_dims, T>>>(DSNewHClr(required_bytes));

                    if (!m_handle)
                    {
                        throw LV_MemoryManagerException(LV_ERR_mZoneErr);
                    }

                    for (int i = 0; i < n_dims; i++)
                    {
                        (*m_handle)->dims[i] = n_elements[i];
                    }
                    return;
                }
                auto current_bytes = DSGetHandleSize(uhandle);

                if (required_bytes > current_bytes)
                {
                    auto err = DSSetHSzClr(uhandle, required_bytes);
                    if (err)
                    {
                        throw LV_MemoryManagerException(err);
                    }
                }

                for (int i = 0; i < n_dims; i++)
                {
                    (*m_handle)->dims[i] = n_elements[i];
                }
            }

        private:
            LV_Handle_t<LV_Array_t<n_dims, T>> m_handle;

            int32_t get_data_index(std::array<int32_t, n_dims> el) const
            {
                int index = 0;
                int stride = 1;

                for (int i = el.size() - 1; i >= 0; i--)
                {
                    index += el[i] * stride;
                    stride *= extents()[i];
                }

                return index;
            }

            bool element_is_in_data_range(std::array<int32_t, n_dims> el) const
            {
                return get_data_index() < get_data_index(extents());
            }

            bool is_valid_handle() const
            {
                return DSCheckHandle(reinterpret_cast<LV_UHandle_t>(m_handle)) == LV_ERR_noError;
            }
        };
    }
}

#include "./reset_packing.hpp"