/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "error.h"
#include "types.h"

#include <algorithm>

template <typename T>
void sort_and_unique(T* v)
{
        std::sort(v->begin(), v->end());
        v->erase(std::unique(v->begin(), v->end()), v->end());
}

template <typename T, typename Less, typename Equal>
void sort_and_unique(T* v, Less less, Equal equal)
{
        std::sort(v->begin(), v->end(), less);
        v->erase(std::unique(v->begin(), v->end(), equal), v->end());
}

// Можно вместо этого использовать std::all_of или std::find(v.cbegin(), v.cend(), true) == v.cend()
template <typename T>
bool all_false(const T& v)
{
        int size = v.size();
        for (int i = 0; i < size; ++i)
        {
                if (v[i])
                {
                        return false;
                }
        }
        return true;
}

// Можно вместо этого использовать std::all_of или std::find(v.cbegin(), v.cend(), false) == v.cend()
template <typename T>
bool all_true(const T& v)
{
        int size = v.size();
        for (int i = 0; i < size; ++i)
        {
                if (!v[i])
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
bool all_non_negative(const T& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const auto& v) { return v >= 0; });
}

template <typename T>
bool all_positive(const T& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const auto& v) { return v > 0; });
}

template <typename T>
bool all_negative(const T& data)
{
        return std::all_of(data.cbegin(), data.cend(), [](const auto& v) { return v < 0; });
}

// Вместо std::accumulate(v.cbegin(), v.cend(), static_cast<T>(1), std::multiplies<void>())
template <typename T, typename V>
constexpr T multiply_all(const V& v)
{
        static_assert((is_native_integral<typename V::value_type> && is_native_integral<T>) ||
                      (is_native_floating_point<typename V::value_type> && is_native_floating_point<T>));
        static_assert(is_signed<typename V::value_type> == is_signed<T>);
        static_assert(limits<typename V::value_type>::digits <= limits<T>::digits);

        if (v.empty())
        {
                error("Empty container for multiply all");
        }

        T value = v[0];
        for (size_t i = 1; i < v.size(); ++i)
        {
                value *= v[i];
        }

        return value;
}

template <typename T, typename V>
constexpr T add_all(const V& v)
{
        static_assert((is_native_integral<typename V::value_type> && is_native_integral<T>) ||
                      (is_native_floating_point<typename V::value_type> && is_native_floating_point<T>));
        static_assert(is_signed<typename V::value_type> == is_signed<T>);
        static_assert(limits<typename V::value_type>::digits <= limits<T>::digits);

        if (v.empty())
        {
                error("Empty container for add all");
        }

        T value = v[0];
        for (size_t i = 1; i < v.size(); ++i)
        {
                value += v[i];
        }

        return value;
}

template <template <typename...> typename Container, typename... T>
void insert_or_erase(bool insert, const typename Container<T...>::value_type& value, Container<T...>* container)
{
        if (insert)
        {
                container->insert(value);
        }
        else
        {
                container->erase(value);
        }
}
