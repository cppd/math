/*
Copyright (C) 2018 Topological Manifold

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

#include <array>
#include <type_traits>
#include <vector>

namespace TypeDetectionImplementation
{
struct IsArray
{
        template <typename T>
        struct S
        {
                static constexpr bool value = false;
        };
        template <typename T, size_t N>
        struct S<std::array<T, N>>
        {
                static constexpr bool value = true;
        };
};

struct IsVector
{
        template <typename T>
        struct S
        {
                static constexpr bool value = false;
        };
        template <typename... Args>
        struct S<std::vector<Args...>>
        {
                static constexpr bool value = true;
        };
};
}

template <typename T>
inline constexpr bool IsArray = TypeDetectionImplementation::IsArray::S<std::remove_cv_t<T>>::value;
template <typename T>
inline constexpr bool IsVector = TypeDetectionImplementation::IsVector::S<std::remove_cv_t<T>>::value;

static_assert(IsArray<const std::array<int, 1>>);
static_assert(IsVector<const std::vector<int>>);

static_assert(!IsArray<const std::vector<int>>);
static_assert(!IsVector<const std::array<int, 1>>);
