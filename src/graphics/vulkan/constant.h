/*
Copyright (C) 2017-2019 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "com/error.h"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
namespace specialization_constant_implementation
{
template <size_t N, typename... T>
class TupleI : TupleI<N - 1, T...>
{
        static_assert(sizeof...(T) > 0);
        static_assert(N > 0 && N <= sizeof...(T));

        using BaseClass = TupleI<N - 1, T...>;

        const std::tuple_element_t<N - 1, std::tuple<T...>> v;

public:
        static constexpr size_t offset = sizeof(BaseClass);

        constexpr TupleI(const std::tuple<T...>& t) : BaseClass(t), v(std::get<N - 1>(t))
        {
        }
};

template <typename... T>
class TupleI<1, T...>
{
        static_assert(sizeof...(T) > 0);

        const std::tuple_element_t<0, std::tuple<T...>> v;

public:
        static constexpr size_t offset = 0;

        constexpr TupleI(const std::tuple<T...>& t) : v(std::get<0>(t))
        {
        }
};

constexpr VkSpecializationMapEntry create_entry(uint32_t id, uint32_t offset, size_t size)
{
        VkSpecializationMapEntry entry = {};
        entry.constantID = id;
        entry.offset = offset;
        entry.size = size;
        return entry;
};

template <typename... T, size_t... I>
std::vector<VkSpecializationMapEntry> create_entries(const std::array<uint32_t, sizeof...(T)>& ids,
                                                     std::integer_sequence<size_t, I...>&&)
{
        static_assert(sizeof...(T) == sizeof...(I));

        return {create_entry(ids[I], TupleI<I + 1, T...>::offset, sizeof(T))...};
}
}

template <typename... T>
using SpecializationConstantTuple = specialization_constant_implementation::TupleI<sizeof...(T), T...>;

template <typename... T>
constexpr SpecializationConstantTuple<T...> create_specialization_constant_tuple(const std::tuple<T...>& data)
{
        namespace impl = specialization_constant_implementation;

        static_assert(((std::is_same_v<int32_t, T> || std::is_same_v<uint32_t, T> || std::is_same_v<float, T> ||
                        std::is_same_v<double, T> || std::is_same_v<VkBool32, T>)&&...));

        return SpecializationConstantTuple<T...>(data);
};

template <typename... T>
std::vector<VkSpecializationMapEntry> create_specialization_constant_entries(const std::array<uint32_t, sizeof...(T)>& ids,
                                                                             const SpecializationConstantTuple<T...>&)
{
        namespace impl = specialization_constant_implementation;

        ASSERT(std::unordered_set<uint32_t>(ids.cbegin(), ids.cend()).size() == ids.size());

        return impl::create_entries<T...>(ids, std::make_integer_sequence<size_t, sizeof...(T)>());
};
}
