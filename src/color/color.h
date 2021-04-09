/*
Copyright (C) 2017-2021 Topological Manifold

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

namespace ns
{
struct Srgb8 final
{
        const unsigned char red, green, blue;

        constexpr Srgb8(unsigned char red, unsigned char green, unsigned char blue) : red(red), green(green), blue(blue)
        {
        }

        bool operator==(const Srgb8& v) const
        {
                return red == v.red && green == v.green && blue == v.blue;
        }
};

class Color final
{
        using T = float;

        Vector<3, T> m_data;

        static_assert(std::is_trivially_copyable_v<Vector<3, T>>);

public:
        using DataType = T;

        Color()
        {
        }

        constexpr explicit Color(T grayscale) : m_data(grayscale)
        {
        }

        constexpr Color(T red, T green, T blue) : m_data(red, green, blue)
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
        [[nodiscard]] std::enable_if_t<std::is_same_v<F, T>, const Vector<3, F>&> rgb() const
        {
                return m_data;
        }

        template <typename F>
        [[nodiscard]] std::enable_if_t<!std::is_same_v<F, T>, Vector<3, F>> rgb() const
        {
                static_assert(std::is_floating_point_v<F>);

                return to_vector<F>(m_data);
        }

        template <typename F>
        void set_rgb(const Vector<3, F>& rgb)
        {
                static_assert(std::is_floating_point_v<F>);

                m_data = to_vector<F>(rgb);
        }

        [[nodiscard]] Srgb8 srgb8() const
        {
                return Srgb8(
                        color::linear_float_to_srgb_uint8<Color::DataType>(m_data[0]),
                        color::linear_float_to_srgb_uint8<Color::DataType>(m_data[1]),
                        color::linear_float_to_srgb_uint8<Color::DataType>(m_data[2]));
        }

        [[nodiscard]] T luminance() const
        {
                return color::linear_float_to_linear_luminance(m_data[0], m_data[1], m_data[2]);
        }

        [[nodiscard]] T max_element() const
        {
                return std::max(m_data[2], std::max(m_data[0], m_data[1]));
        }

        [[nodiscard]] bool below(T v) const
        {
                return m_data[0] < v && m_data[1] < v && m_data[2] < v;
        }

        [[nodiscard]] T red() const
        {
                return m_data[0];
        }

        [[nodiscard]] T green() const
        {
                return m_data[1];
        }

        [[nodiscard]] T blue() const
        {
                return m_data[2];
        }

        void clamp()
        {
                for (int i = 0; i < 3; ++i)
                {
                        m_data[i] = std::clamp(m_data[i], T(0), T(1));
                }
        }

        [[nodiscard]] Color clamped() const
        {
                Color c;
                for (int i = 0; i < 3; ++i)
                {
                        c.m_data[i] = std::clamp(m_data[i], T(0), T(1));
                }
                return c;
        }

        template <typename F>
        [[nodiscard]] Color interpolation(const Color& c, F x) const
        {
                Color r;
                r.m_data = ::ns::interpolation(m_data, c.m_data, x);
                return r;
        }

        [[nodiscard]] bool operator==(const Color& c) const
        {
                return m_data == c.m_data;
        }

        Color& operator+=(const Color& c)
        {
                m_data += c.m_data;
                return *this;
        }

        Color& operator-=(const Color& c)
        {
                m_data -= c.m_data;
                return *this;
        }

        Color& operator*=(const Color& c)
        {
                m_data *= c.m_data;
                return *this;
        }

        template <typename F>
        Color& operator*=(F b)
        {
                m_data *= static_cast<Color::DataType>(b);
                return *this;
        }

        template <typename F>
        Color& operator/=(F b)
        {
                m_data /= static_cast<Color::DataType>(b);
                return *this;
        }
};

template <typename F>
[[nodiscard]] Color interpolation(const Color& a, const Color& b, F x)
{
        return a.interpolation(b, x);
}

[[nodiscard]] inline Color operator+(const Color& a, const Color& b)
{
        return Color(a) += b;
}

[[nodiscard]] inline Color operator-(const Color& a, const Color& b)
{
        return Color(a) -= b;
}

template <typename F>
[[nodiscard]] Color operator*(const Color& a, F b)
{
        return Color(a) *= b;
}

template <typename F>
[[nodiscard]] Color operator*(F b, const Color& a)
{
        return Color(a) *= b;
}

[[nodiscard]] inline Color operator*(const Color& a, const Color& b)
{
        return Color(a) *= b;
}

template <typename F>
[[nodiscard]] Color operator/(const Color& a, F b)
{
        return Color(a) /= b;
}
}
