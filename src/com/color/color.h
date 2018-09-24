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

#include "conversion.h"

#include "com/vec.h"

#include <algorithm>

struct SrgbInteger
{
        unsigned char red, green, blue;

        constexpr SrgbInteger(unsigned char red_, unsigned char green_, unsigned char blue_)
                : red(red_), green(green_), blue(blue_)
        {
        }
};

class Color
{
        using T = float;

        Vector<3, T> m_data;

public:
        using DataType = T;

        Color() = default;

        explicit Color(T grayscale) : m_data(grayscale)
        {
        }

        explicit Color(Vector<3, T>&& rgb) : m_data(std::move(rgb))
        {
        }

        Color(const SrgbInteger& c)
                : m_data(color_conversion::srgb_integer_to_rgb_float<T>(c.red),
                         color_conversion::srgb_integer_to_rgb_float<T>(c.green),
                         color_conversion::srgb_integer_to_rgb_float<T>(c.blue))
        {
        }

        template <typename F>
        std::enable_if_t<std::is_same_v<F, T>, const Vector<3, F>&> to_rgb_vector() const
        {
                return m_data;
        }
        template <typename F>
        std::enable_if_t<!std::is_same_v<F, T>, Vector<3, F>> to_rgb_vector() const
        {
                static_assert(std::is_floating_point_v<F>);

                return to_vector<F>(m_data);
        }

        T luminance() const
        {
                return color_conversion::rgb_float_to_rgb_luminance(m_data[0], m_data[1], m_data[2]);
        }

        T max_element() const
        {
                return std::max(m_data[2], std::max(m_data[0], m_data[1]));
        }

        Vector<3, T>& data()
        {
                return m_data;
        }

        const Vector<3, T>& data() const
        {
                return m_data;
        }

        T red() const
        {
                return m_data[0];
        }
        T green() const
        {
                return m_data[1];
        }
        T blue() const
        {
                return m_data[2];
        }

        void operator+=(const Color& c)
        {
                m_data += c.m_data;
        }
};

template <typename F>
Color interpolation(const Color& a, const Color& b, F x)
{
        return Color(interpolation(a.data(), b.data(), x));
}

template <typename F>
Color operator*(const Color& a, F b)
{
        return Color(a.data() * static_cast<Color::DataType>(b));
}

template <typename F>
Color operator*(F b, const Color& a)
{
        return Color(static_cast<Color::DataType>(b) * a.data());
}

template <typename F>
Color operator/(const Color& a, F b)
{
        return Color(a.data() / static_cast<Color::DataType>(b));
}

inline Color operator+(const Color& a, const Color& b)
{
        return Color(a.data() + b.data());
}

inline Color operator*(const Color& a, const Color& b)
{
        return Color(a.data() * b.data());
}
