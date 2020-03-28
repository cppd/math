/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/com/type/trait.h>
#include <src/numerical/vec.h>
#include <src/utility/string/ascii.h>

#include <string>

namespace mesh::file
{
constexpr bool is_hyphen_minus(char c)
{
        return c == '-';
}

template <typename Data, typename Op>
void read(const Data& data, long long size, const Op& op, long long* i)
{
        while (*i < size && op(data[*i]))
        {
                ++(*i);
        }
}

// Между begin и end находится уже проверенное целое число в формате DDDDD без знака
template <typename Integer, typename T>
Integer digits_to_integer(const T& data, long long begin, long long end)
{
        static_assert(is_native_integral<Integer>);

        long long length = end - begin;

        if (length > limits<Integer>::digits10 || length < 1)
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

template <typename T, typename Integer>
bool read_integer(const T& data, long long size, long long* pos, Integer* value)
{
        static_assert(is_signed<Integer>);

        long long begin = *pos;

        if (begin < size && is_hyphen_minus(data[begin]))
        {
                ++begin;
        }

        long long end = begin;

        read(data, size, ascii::is_digit, &end);

        if (end > begin)
        {
                *value = (begin == *pos) ? digits_to_integer<Integer>(data, begin, end) :
                                           -digits_to_integer<Integer>(data, begin, end);
                *pos = end;

                return true;
        }

        return false;
}

template <typename T>
bool read_one_float_from_string(const char** str, T* p)
{
        using FP = std::remove_volatile_t<T>;

        static_assert(std::is_same_v<FP, float> || std::is_same_v<FP, double> || std::is_same_v<FP, long double>);

        char* end;

        if constexpr (std::is_same_v<FP, float>)
        {
                *p = std::strtof(*str, &end);
        }
        if constexpr (std::is_same_v<FP, double>)
        {
                *p = std::strtod(*str, &end);
        }
        if constexpr (std::is_same_v<FP, long double>)
        {
                *p = std::strtold(*str, &end);
        }

        // В соответствии со спецификацией файла OBJ, между числами должны быть пробелы,
        // а после чисел пробелы, конец строки или комментарий.
        // Здесь без проверок этого.
        if (*str == end || errno == ERANGE || !is_finite(*p))
        {
                return false;
        }
        *str = end;
        return true;
};

template <typename... T>
int string_to_floats(const char* str, T*... floats)
{
        constexpr int N = sizeof...(T);

        static_assert(N > 0);
        static_assert(((
                std::is_same_v<std::remove_volatile_t<T>, float> || std::is_same_v<std::remove_volatile_t<T>, double> ||
                std::is_same_v<std::remove_volatile_t<T>, long double>)&&...));

        errno = 0;
        int cnt = 0;

        ((read_one_float_from_string(&str, floats) ? ++cnt : false) && ...);

        return cnt;
}

template <size_t N, typename T, unsigned... I>
int read_vector(const char* str, Vector<N, T>* v, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));

        return string_to_floats(str, &(*v)[I]...);
}

template <size_t N, typename T, unsigned... I>
int read_vector(const char* str, Vector<N, T>* v, T* n, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));

        return string_to_floats(str, &(*v)[I]..., n);
}

template <size_t N, typename T>
void read_float(const char* str, Vector<N, T>* v)
{
        if (N != read_vector(str, v, std::make_integer_sequence<unsigned, N>()))
        {
                error(std::string("Error read " + to_string(N) + " floating points of ") + type_name<T>() + " type");
        }
}

template <typename T>
void read_float(const char* str, T* v)
{
        static_assert(std::is_floating_point_v<T>);

        if (1 != string_to_floats(str, v))
        {
                error(std::string("Error read 1 floating point of ") + type_name<T>() + " type");
        }
}
}
