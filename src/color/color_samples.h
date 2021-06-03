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

#include <src/com/interpolation.h>
#include <src/com/type/limit.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ns::color
{
template <typename Derived, std::size_t N, typename T>
class ColorSamples
{
        static_assert(std::is_floating_point_v<T>);

protected:
        Vector<N, T> m_data;

        static constexpr std::size_t SIZE = N;
        using DataType = T;

        template <typename... Args>
        constexpr explicit ColorSamples(Args... args) : m_data{args...}
        {
                static_assert(std::is_base_of_v<ColorSamples, Derived>);
                static_assert(std::is_final_v<Derived>);
                static_assert(sizeof(ColorSamples) == sizeof(Derived));
                static_assert(std::is_trivially_copyable_v<Derived>);
                static_assert(std::is_trivially_destructible_v<Derived>);
        }

        ~ColorSamples() = default;

        [[nodiscard]] std::string to_string(const std::string_view& name) const
        {
                std::ostringstream oss;
                oss.precision(limits<T>::max_digits10);
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
        template <typename F>
        void multiply_add(const Derived& a, F b)
        {
                m_data.multiply_add(a.m_data, static_cast<T>(b));
        }

        template <typename F>
        void multiply_add(F b, const Derived& a)
        {
                multiply_add(a, b);
        }

        //void clamp()
        //{
        //        for (std::size_t i = 0; i < N; ++i)
        //        {
        //                m_data[i] = std::clamp(m_data[i], T(0), T(1));
        //        }
        //}

        [[nodiscard]] Derived clamped() const
        {
                Derived c;
                for (std::size_t i = 0; i < N; ++i)
                {
                        c.m_data[i] = std::clamp(m_data[i], T(0), T(1));
                }
                return c;
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

        //

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

        //

        template <typename F>
        [[nodiscard]] friend Derived interpolation(const Derived& a, const Derived& b, F x)
        {
                Derived r;
                r.m_data = ::ns::interpolation(a.m_data, b.m_data, x);
                return r;
        }

        [[nodiscard]] friend Derived operator+(const Derived& a, const Derived& b)
        {
                return Derived(a) += b;
        }

        [[nodiscard]] friend Derived operator-(const Derived& a, const Derived& b)
        {
                return Derived(a) -= b;
        }

        template <typename F>
        [[nodiscard]] friend Derived operator*(const Derived& a, F b)
        {
                return Derived(a) *= b;
        }

        template <typename F>
        [[nodiscard]] friend Derived operator*(F b, const Derived& a)
        {
                return Derived(a) *= b;
        }

        [[nodiscard]] friend Derived operator*(const Derived& a, const Derived& b)
        {
                return Derived(a) *= b;
        }

        template <typename F>
        [[nodiscard]] friend Derived operator/(const Derived& a, F b)
        {
                return Derived(a) /= b;
        }
};
}
