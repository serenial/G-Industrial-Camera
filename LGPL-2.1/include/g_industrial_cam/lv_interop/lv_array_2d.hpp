#pragma once

#include <vector>
#include <array>
#include <functional>
#include <cstring>
#include <numeric>
#include <exception>
#include <algorithm>
#include <cstddef>

#include "./lv_types.hpp"
#include "./lv_functions.hpp"
#include "./lv_array_1d.hpp"

#include "./set_packing.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {

        template <class T>
        class LV_2DArrayHandle_t
        {
        public:
            LV_2DArrayHandle_t() : m_handle(nullptr) {}

            std::array<int32_t, 2> extents() const
            {
                if (m_handle && (*m_handle))
                {
                    return std::array<int32_t, 2>{{(*m_handle)->dims[0], (*m_handle)->dims[1]}};
                }
                return std::array<int32_t, 2>{{0, 0}};
            }

            T &operator[](std::array<int32_t, 2> el)
            {
                if (m_handle && element_is_in_data_range(el))
                {
                    return (*m_handle)->data[get_data_index(el)];
                }
                throw std::invalid_argument("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            T const &operator[](std::array<int32_t, 2> el) const
            {
                if (m_handle && element_is_in_data_range(el))
                {
                    return (*m_handle)->data[get_data_index(el)];
                }
                throw std::invalid_argument("Attempting to access LabVIEW Array Handle outside of bounds.");
            }

            T *data_handle() const noexcept
            {
                return (*m_handle)->data_ptr();
            }

            void dispose()
            {
                if (m_handle)
                {
                    DSDisposeHandle(reinterpret_cast<LV_UHandle_t>(m_handle));
                    m_handle = nullptr;
                }
            }

            void size_to_fit(std::array<int32_t, 2> n_elements, std::function<void(T)> deallocator = [](T el) {})
            {

                auto bytes = required_bytes(n_elements);
                auto uhandle = reinterpret_cast<LV_UHandle_t>(m_handle);

                if (DSCheckHandle(uhandle) == LV_ERR_mZoneErr)
                {
                    m_handle = reinterpret_cast<LV_Handle_t<LV_Array_t<2, T>>>(DSNewHandle(bytes));
                    (*m_handle)->dims[0] = n_elements[0];
                    (*m_handle)->dims[1] = n_elements[1];
                    return;
                }
                auto current_bytes = DSGetHandleSize(uhandle);

                if (bytes > current_bytes)
                {
                    auto err = DSSetHandleSize(uhandle, bytes);
                    if (err)
                    {
                        throw LV_MemoryManagerException(err);
                    }
                }

                auto old_extents = extents();

                for (int32_t row = n_elements[0]; row < old_extents[0]; row++)
                {
                    for (int32_t col = n_elements[1]; col < old_extents[1]; col++)
                    {
                        deallocator((*m_handle)->data[get_data_index({row, col})]);
                    }
                }

                (*m_handle)->dims[0] = n_elements[0];
                (*m_handle)->dims[1] = n_elements[1];
            }

        private:
            LV_Handle_t<LV_Array_t<2, T>> m_handle;

            size_t required_bytes(std::array<int32_t, 2> n_elements)
            {
                return LV_Array_t<2, T>::data_member_offset_bytes() + sizeof(T) * n_elements[0] * n_elements[1];
            }

            int32_t get_data_index(std::array<int32_t, 2> el)
            {
                return extents()[1] * el[0] + el[1];
            }

            bool element_is_in_data_range(std::array<int32_t, 2> el)
            {
                return get_data_index < get_data_index(extents());
            }
        };
    }
}

#include "./reset_packing.hpp"