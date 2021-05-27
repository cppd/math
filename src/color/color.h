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

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ns
{
struct RGB8 final
{
        const unsigned char red, green, blue;

        constexpr RGB8(unsigned char red, unsigned char green, unsigned char blue) : red(red), green(green), blue(blue)
        {
        }

        bool operator==(const RGB8& v) const
        {
                return red == v.red && green == v.green && blue == v.blue;
        }
};

namespace color_implementation
{
template <typename Derived, std::size_t N, typename T>
class Samples
{
        static_assert(std::is_floating_point_v<T>);

protected:
        static constexpr std::size_t SIZE = N;
        using DataType = T;

        Vector<N, T> m_data;

        template <typename... Args>
        constexpr explicit Samples(Args... args) : m_data{args...}
        {
                static_assert(std::is_base_of_v<Samples, Derived>);
                static_assert(std::is_final_v<Derived>);
                static_assert(sizeof(Samples) == sizeof(Derived));
                static_assert(std::is_trivially_copyable_v<Derived>);
        }

        ~Samples() = default;

        [[nodiscard]] std::string to_string(const std::string_view& name) const
        {
                std::ostringstream oss;
                oss.precision(limits<T>::max_digits10);
                static_assert(N == 3);
                oss << name;
                oss << '(';
                oss << m_data[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        oss << ", " << m_data[i];
                }
                oss << ')';
                return oss.str();
        }

public:
        void clamp()
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        m_data[i] = std::clamp(m_data[i], T(0), T(1));
                }
        }

        [[nodiscard]] Derived clamped() const
        {
                Derived c;
                for (std::size_t i = 0; i < N; ++i)
                {
                        c.m_data[i] = std::clamp(m_data[i], T(0), T(1));
                }
                return c;
        }

        template <typename F>
        [[nodiscard]] Derived interpolation(const Derived& c, F x) const
        {
                Derived r;
                r.m_data = ::ns::interpolation(m_data, c.m_data, x);
                return r;
        }

        [[nodiscard]] bool operator==(const Derived& c) const
        {
                return m_data == c.m_data;
        }

        Derived& operator+=(const Derived& c)
        {
                m_data += c.m_data;
                return *static_cast<Derived*>(this);
        }

        Derived& operator-=(const Derived& c)
        {
                m_data -= c.m_data;
                return *static_cast<Derived*>(this);
        }

        Derived& operator*=(const Derived& c)
        {
                m_data *= c.m_data;
                return *static_cast<Derived*>(this);
        }

        template <typename F>
        Derived& operator*=(F b)
        {
                m_data *= static_cast<T>(b);
                return *static_cast<Derived*>(this);
        }

        template <typename F>
        Derived& operator/=(F b)
        {
                m_data /= static_cast<T>(b);
                return *static_cast<Derived*>(this);
        }

        [[nodiscard]] bool is_black() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(m_data[0] <= 0))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool has_nan() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (std::isnan(m_data[i]))
                        {
                                return true;
                        }
                }
                return false;
        }

        [[nodiscard]] bool is_finite() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!std::isfinite(m_data[i]))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool is_non_negative() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(m_data[i] >= 0))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool is_in_range(T low, T high) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(m_data[i] >= low && m_data[i] <= high))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool equal_to(const Derived& c, T relative_error) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        T c1 = m_data[i];
                        T c2 = c.m_data[i];
                        if (c1 == c2)
                        {
                                continue;
                        }
                        T max = std::max(std::abs(c1), std::abs(c2));
                        if (!(std::abs(c1 - c2) / max < relative_error))
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool less_than(const Derived& c, T relative_error) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        T c1 = m_data[i];
                        T c2 = c.m_data[i];
                        if (c1 <= c2)
                        {
                                continue;
                        }
                        T max = std::max(std::abs(c1), std::abs(c2));
                        if (!(std::abs(c1 - c2) / max < relative_error))
                        {
                                return false;
                        }
                }
                return true;
        }
};
}

class RGB final : public color_implementation::Samples<RGB, 3, float>
{
        static constexpr std::size_t N = SIZE;
        using T = DataType;
        using Base = Samples<RGB, N, T>;

public:
        using DataType = T;

        RGB()
        {
        }

        constexpr explicit RGB(T v) : Samples(v)
        {
        }

        constexpr RGB(T red, T green, T blue) : Samples(red, green, blue)
        {
        }

        constexpr explicit RGB(const RGB8& c)
                : Samples(
                        color::srgb_uint8_to_linear_float(c.red),
                        color::srgb_uint8_to_linear_float(c.green),
                        color::srgb_uint8_to_linear_float(c.blue))
        {
        }

        template <typename F>
        [[nodiscard]] std::enable_if_t<std::is_same_v<F, T>, const Vector<3, T>&> rgb() const
        {
                return m_data;
        }

        template <typename F>
        [[nodiscard]] std::enable_if_t<!std::is_same_v<F, T>, Vector<3, F>> rgb() const
        {
                static_assert(std::is_floating_point_v<F>);
                return to_vector<F>(m_data);
        }

        [[nodiscard]] RGB8 rgb8() const
        {
                return RGB8(
                        color::linear_float_to_srgb_uint8<T>(m_data[0]),
                        color::linear_float_to_srgb_uint8<T>(m_data[1]),
                        color::linear_float_to_srgb_uint8<T>(m_data[2]));
        }

        template <typename F>
        void set_rgb(const Vector<3, F>& rgb)
        {
                static_assert(std::is_floating_point_v<F>);
                m_data = to_vector<F>(rgb);
        }

        [[nodiscard]] T luminance() const
        {
                return color::linear_float_to_linear_luminance(m_data[0], m_data[1], m_data[2]);
        }

        [[nodiscard]] std::string to_string() const
        {
                return Base::to_string("rgb");
        }
};

using Color = RGB;

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

[[nodiscard]] inline std::string to_string(const Color& c)
{
        return c.to_string();
}
}
