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

#include "error.h"

#include "com/type/limit.h"
#include "com/type/trait.h"

#include <algorithm>
#include <type_traits>

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

template <typename T>
std::remove_cv_t<std::remove_reference_t<T>> unique_elements(T&& v)
{
        std::remove_cv_t<std::remove_reference_t<T>> res(std::forward<T>(v));
        sort_and_unique(&res);
        return res;
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

template <typename T1, typename T2>
bool there_is_intersection(T1 t1, T2 t2)
{
        std::sort(t1.begin(), t1.end());
        std::sort(t2.begin(), t2.end());

        auto i1 = std::cbegin(t1);
        auto i2 = std::cbegin(t2);

        while (true)
        {
                if (i1 == std::cend(t1))
                {
                        return false;
                }
                if (i2 == std::cend(t2))
                {
                        return false;
                }

                if (*i1 < *i2)
                {
                        ++i1;
                }
                else if (*i1 > *i2)
                {
                        ++i2;
                }
                else
                {
                        // Из условия !(a < b) && !(a > b) не обязательно следует a == b
                        if (!(*i1 == *i2))
                        {
                                error("Failed to find intersection. Not (a < b) and not (a > b) and not (a == b)");
                        }

                        return true;
                }
        }
}
