/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/string/ascii.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace ns::mesh::file
{
namespace data_read_implementation
{
template <typename Integer, typename Iter>
Integer digits_to_integer(const Iter first, Iter last)
{
        static_assert(std::is_integral_v<Integer>);

        if (const auto length = last - first; length > Limits<Integer>::digits10() || length < 1)
        {
                error("Error converting " + std::string(first, last) + " to integral");
        }

        --last;
        Integer sum = ascii::char_to_int(*last);
        Integer mul = 1;
        while (last-- > first)
        {
                mul *= 10;
                sum += ascii::char_to_int(*last) * mul;
        }

        return sum;
}

template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, float>)
{
        char* end;
        const T v = std::strtof(str, &end);
        return {v, end};
}

template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, double>)
{
        char* end;
        const T v = std::strtod(str, &end);
        return {v, end};
}

template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, long double>)
{
        char* end;
        const T v = std::strtold(str, &end);
        return {v, end};
}

template <typename T>
bool read_one_float_from_string(const char** const str, T* const p)
{
        const auto [value, end] = str_to_floating_point<T>(*str);

        if (end == *str || errno == ERANGE || !std::isfinite(value))
        {
                return false;
        }

        if (!(ascii::is_space(*end) || *end == '\0' || *end == '#'))
        {
                return false;
        }

        *p = value;
        *str = end;
        return true;
}

template <typename... T>
int string_to_floats(const char** const str, T* const... floats)
{
        static_assert(sizeof...(T) > 0);
        static_assert(((std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, long double>)&&...));

        errno = 0;
        int cnt = 0;

        ((read_one_float_from_string(str, floats) ? ++cnt : false) && ...);

        return cnt;
}

template <std::size_t N, typename T, unsigned... I>
int read_vector(const char** const str, Vector<N, T>* const v, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));
        return string_to_floats(str, &(*v)[I]...);
}

template <std::size_t N, typename T, unsigned... I>
int read_vector(const char** const str, Vector<N, T>* const v, T* const n, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));
        return string_to_floats(str, &(*v)[I]..., n);
}
}

//

template <typename Iter, typename Op>
[[nodiscard]] Iter read(Iter first, const Iter last, const Op& op)
{
        while (first < last && op(*first))
        {
                ++first;
        }
        return first;
}

template <typename T, typename Iter>
std::tuple<std::optional<T>, Iter> read_integer(const Iter first, const Iter last)
{
        namespace impl = data_read_implementation;

        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

        const Iter i1 = (first < last && *first == '-') ? first + 1 : first;
        const Iter i2 = read(i1, last, ascii::is_digit);

        if (i2 == i1)
        {
                return {std::nullopt, first};
        }

        const T v = impl::digits_to_integer<T>(i1, i2);
        return {(first == i1) ? v : -v, i2};
}

template <std::size_t N, typename T>
const char* read_float(const char* str, Vector<N, T>* const v, std::optional<T>* const n)
{
        namespace impl = data_read_implementation;

        T t;
        const int cnt = impl::read_vector(&str, v, &t, std::make_integer_sequence<unsigned, N>());

        switch (cnt)
        {
        case N:
                *n = std::nullopt;
                break;
        case N + 1:
                *n = t;
                break;
        default:
                error("Error reading " + std::to_string(N) + " or " + std::to_string(N + 1)
                      + " floating point numbers of " + type_name<T>() + " type, found " + std::to_string(cnt)
                      + " numbers");
        }

        return str;
}

template <std::size_t N, typename T>
const char* read_float(const char* str, Vector<N, T>* const v)
{
        namespace impl = data_read_implementation;

        const int cnt = impl::read_vector(&str, v, std::make_integer_sequence<unsigned, N>());

        if (N != cnt)
        {
                error("Error reading " + std::to_string(N) + " floating point numbers of " + type_name<T>()
                      + " type, found " + std::to_string(cnt) + " numbers");
        }

        return str;
}

template <typename... Args>
const char* read_float(const char* str, Args* const... args) requires(
        (sizeof...(Args) > 0) && (std::is_floating_point_v<Args> && ...))
{
        namespace impl = data_read_implementation;

        static constexpr int N = sizeof...(Args);
        static_assert(N > 0);
        using T = std::tuple_element_t<0, std::tuple<Args...>>;
        static_assert((std::is_floating_point_v<Args> && ...));
        static_assert((std::is_same_v<T, Args> && ...));

        const int cnt = impl::string_to_floats(&str, args...);

        if (N != cnt)
        {
                error("Error reading " + std::to_string(N) + " floating point numbers of " + type_name<T>()
                      + " type, found " + std::to_string(cnt) + " numbers");
        }

        return str;
}
}
