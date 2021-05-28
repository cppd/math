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

#include "color_samples.h"
#include "conversion.h"

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

class RGB final : public color::ColorSamples<RGB, 3, float>
{
        using T = ColorSamples::DataType;

public:
        using DataType = ColorSamples::DataType;

        RGB()
        {
        }

        constexpr explicit RGB(T v) : ColorSamples(v)
        {
        }

        constexpr RGB(T red, T green, T blue) : ColorSamples(red, green, blue)
        {
        }

        constexpr explicit RGB(const RGB8& c)
                : ColorSamples(
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

        [[nodiscard]] friend std::string to_string(const RGB& c)
        {
                return c.to_string("rgb");
        }
};

using Color = RGB;
}
