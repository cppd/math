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

#include "conversion.h"

#include <src/numerical/vec.h>

struct Srgb8
{
        const unsigned char red, green, blue;

        constexpr Srgb8(unsigned char r, unsigned char g, unsigned char b) : red(r), green(g), blue(b)
        {
        }

        bool operator==(const Srgb8& v) const
        {
                return red == v.red && green == v.green && blue == v.blue;
        }
};

class Color
{
        using T = float;

        Vector<3, T> m_data;

        static_assert(std::is_trivially_copyable_v<Vector<3, T>>);

public:
        using DataType = T;

        Color() = default;

        constexpr explicit Color(T grayscale) : m_data(grayscale)
        {
        }

        constexpr explicit Color(Vector<3, T>&& rgb) : m_data(rgb)
        {
        }

        constexpr explicit Color(const Srgb8& c)
                : m_data(
                        color::srgb_uint8_to_linear_float(c.red),
                        color::srgb_uint8_to_linear_float(c.green),
                        color::srgb_uint8_to_linear_float(c.blue))
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

        Srgb8 to_srgb8() const
        {
                unsigned char r = color::linear_float_to_srgb_uint8<Color::DataType>(red());
                unsigned char g = color::linear_float_to_srgb_uint8<Color::DataType>(green());
                unsigned char b = color::linear_float_to_srgb_uint8<Color::DataType>(blue());
                return Srgb8(r, g, b);
        }

        T luminance() const
        {
                return color::linear_float_to_linear_luminance(m_data[0], m_data[1], m_data[2]);
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

        bool operator==(const Color& c) const
        {
                return m_data == c.m_data;
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
