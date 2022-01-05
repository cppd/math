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
#include <string>
#include <vector>

namespace ns::mesh::file
{
namespace data_read_implementation
{
template <typename Integer, typename T>
Integer digits_to_integer(const T& data, const long long begin, long long end)
{
        static_assert(std::is_integral_v<Integer>);

        long long length = end - begin;

        if (length > Limits<Integer>::digits10() || length < 1)
        {
                error("Error convert " + std::string(&data[begin], length) + " to integral");
        }

        --end;
        Integer sum = ascii::char_to_int(data[end]);
        Integer mul = 1;
        while (--end >= begin)
        {
                mul *= 10;
                sum += ascii::char_to_int(data[end]) * mul;
        }

        return sum;
}

template <typename T>
T str_to_floating_point(const char* const str, char** const end) requires(std::is_same_v<T, float>)
{
        return std::strtof(str, end);
}
template <typename T>
T str_to_floating_point(const char* const str, char** const end) requires(std::is_same_v<T, double>)
{
        return std::strtod(str, end);
}
template <typename T>
T str_to_floating_point(const char* const str, char** const end) requires(std::is_same_v<T, long double>)
{
        return std::strtold(str, end);
}

template <typename T>
bool read_one_float_from_string(const char** const str, T* const p)
{
        char* end;
        T value = str_to_floating_point<T>(*str, &end);

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

// split string into two parts
// 1. not space characters
// 2. all other characters before a comment or the end of the string
inline void split(
        const std::vector<char>& data,
        const long long first,
        const long long last,
        long long* const first_b,
        long long* const first_e,
        long long* const second_b,
        long long* const second_e)
{
        const auto is_comment = [](char c)
        {
                return c == '#';
        };

        long long i = first;

        while (i < last && ascii::is_space(data[i]))
        {
                ++i;
        }
        if (i == last || is_comment(data[i]))
        {
                *first_b = i;
                *first_e = i;
                *second_b = i;
                *second_e = i;
                return;
        }

        long long i2 = i + 1;
        while (i2 < last && !ascii::is_space(data[i2]) && !is_comment(data[i2]))
        {
                ++i2;
        }
        *first_b = i;
        *first_e = i2;

        i = i2;

        if (i == last || is_comment(data[i]))
        {
                *second_b = i;
                *second_e = i;
                return;
        }

        // skip the first space
        ++i;

        i2 = i;
        while (i2 < last && !is_comment(data[i2]))
        {
                ++i2;
        }

        *second_b = i;
        *second_e = i2;
}
}

//

constexpr bool str_equal(const char* s1, const char* s2)
{
        while (*s1 == *s2 && *s1)
        {
                ++s1;
                ++s2;
        }
        return *s1 == *s2;
}

static_assert(
        str_equal("ab", "ab") && str_equal("", "") && !str_equal("", "ab") && !str_equal("ab", "")
        && !str_equal("ab", "ac") && !str_equal("ba", "ca") && !str_equal("a", "xyz"));

template <typename T1, typename T2, typename T3>
bool check_range(const T1& v, const T2& min, const T3& max)
{
        return v >= min && v <= max;
}

template <typename Data, typename Op>
void read(const Data& data, const long long size, const Op& op, long long* const i)
{
        while (*i < size && op(data[*i]))
        {
                ++(*i);
        }
}

template <typename T, typename Integer>
bool read_integer(const T& data, const long long size, long long* const pos, Integer* const value)
{
        static_assert(std::is_signed_v<Integer>);
        namespace impl = data_read_implementation;

        long long begin = *pos;

        if (begin < size && data[begin] == '-')
        {
                ++begin;
        }

        long long end = begin;

        read(data, size, ascii::is_digit, &end);

        if (end > begin)
        {
                *value = (begin == *pos) ? impl::digits_to_integer<Integer>(data, begin, end)
                                         : -impl::digits_to_integer<Integer>(data, begin, end);
                *pos = end;

                return true;
        }

        return false;
}

template <std::size_t N, typename T>
[[nodiscard]] std::pair<int, const char*> read_float(const char* str, Vector<N, T>* const v, T* const n)
{
        namespace impl = data_read_implementation;

        const int cnt = impl::read_vector(&str, v, n, std::make_integer_sequence<unsigned, N>());
        return {cnt, str};
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

inline void split_line(
        std::vector<char>* const data,
        const std::vector<long long>& line_begin,
        const long long line_num,
        const char** const first,
        const char** const second,
        long long* const second_b,
        long long* const second_e)
{
        long long line_count = line_begin.size();

        long long last = (line_num + 1 < line_count) ? line_begin[line_num + 1] : data->size();

        // move to '\n' at the end of the string
        --last;

        long long first_b;
        long long first_e;

        data_read_implementation::split(*data, line_begin[line_num], last, &first_b, &first_e, second_b, second_e);

        *first = &(*data)[first_b];
        (*data)[first_e] = 0; // space, '#', '\n'

        *second = &(*data)[*second_b];
        (*data)[*second_e] = 0; // '#', '\n'
}

template <typename T>
void read_name(
        const char* const object_name,
        const T& data,
        const long long begin,
        const long long end,
        std::string* const name)
{
        const long long size = end;

        long long i = begin;
        read(data, size, ascii::is_space, &i);
        if (i == size)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        long long i2 = i;
        read(data, size, ascii::is_not_space, &i2);

        *name = std::string(&data[i], i2 - i);

        i = i2;

        read(data, size, ascii::is_space, &i);
        if (i != size)
        {
                error("Error read " + std::string(object_name) + " name");
        }
}
}
