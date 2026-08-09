#pragma once

#include <vector>
#include <array>
#include <functional>
#include <cstring>
#include <numeric>
#include <exception>
#include <algorithm>
#include <stdexcept>

#include "./lv_types.hpp"
#include "./lv_functions.hpp"
#include "./lv_array_md.hpp"

#include "./set_packing.hpp"

namespace g_industrial_cam
{
    namespace lv_interop
    {
        template <class T>
        class LV_1DArrayHandle_t : public LV_MDArrayHandle_t<1, T>
        {

        public:
            LV_1DArrayHandle_t() = delete;

            bool empty() const
            {
                return size() == 0;
            }

            std::size_t size() const
            {
                return LV_MDArrayHandle_t<1, T>::extents()[0];
            }

            void size_to_fit(size_t n)
            {
                LV_MDArrayHandle_t<1, T>::size_to_fit({static_cast<int32_t>(n)});
            }

            T &operator[](int32_t el)
            {
                return LV_MDArrayHandle_t<1, T>::operator[](std::array<int32_t,1>{el});
            }

            T const &operator[](int32_t el) const
            {
                return LV_MDArrayHandle_t<1, T>::operator[](std::array<int32_t,1>{el});
            }

            // copy from for non trivial types that requires each element to be copied in turn
            template <class ElementType, typename CopyFunction = std::function<void(const ElementType &, T *)>>
            void copy_element_by_element_from(std::vector<ElementType> vector, CopyFunction copy_fn = [](const auto &from, auto to)
                                                                               { *to = from; })
            {
                size_to_fit(vector.size());

                auto vector_it = vector.begin();
                auto array_it = LV_MDArrayHandle_t<1, T>::begin();

                for (; vector_it != vector.end() && array_it != LV_MDArrayHandle_t<1, T>::end(); ++vector_it, ++array_it)
                {
                    copy_fn(*vector_it, array_it);
                }
            }

            // copy from for contiguous fundamental types which can be copied in a block
            template <class ElementType>
            void copy_memory_from(const std::vector<ElementType> &vector)
            {
                // memcpy only works with fundamental types
                static_assert(std::is_trivially_copyable<ElementType>::value == true, "Default copying is only compatible for fundamental value types. Specify the copy function [](auto from, auto to){ /* do conversion */}");
                size_to_fit(vector.size());
                std::memcpy(LV_MDArrayHandle_t<1, T>::begin(), vector.data(), vector.size() * sizeof(T));
            }

            template <typename T_vec>
            const std::vector<T_vec> to_vector() const
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
        };
    }
}

#include "./reset_packing.hpp"
