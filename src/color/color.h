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
#include "rgb_samples.h"
#include "xyz_samples.h"

#include <src/com/error.h>

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

template <int FROM, int TO, std::size_t N, typename T>
class Spectrum final : public color::ColorSamples<Spectrum<FROM, TO, N, T>, N, T>
{
        using Base = color::ColorSamples<Spectrum<FROM, TO, N, T>, N, T>;

        static_assert(N > 0);

        struct Colors
        {
                std::array<T, N> white;
                std::array<T, N> cyan;
                std::array<T, N> magenta;
                std::array<T, N> yellow;
                std::array<T, N> red;
                std::array<T, N> green;
                std::array<T, N> blue;
        };

        struct Samples
        {
                std::array<T, N> x;
                std::array<T, N> y;
                std::array<T, N> z;
                Colors reflectance;
                Colors illumination;
        };

        static Samples create_samples()
        {
                const auto copy = [](std::array<T, N>* dst, std::vector<T>&& src)
                {
                        ASSERT(src.size() == N);
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                (*dst)[i] = src[i];
                        }
                };

                Samples samples;

                copy(&samples.x, color::cie_x_samples<color::XYZ_31>(FROM, TO, N));
                copy(&samples.y, color::cie_y_samples<color::XYZ_31>(FROM, TO, N));
                copy(&samples.x, color::cie_z_samples<color::XYZ_31>(FROM, TO, N));

                Colors& reflectance = samples.reflectance;
                copy(&reflectance.white, color::rgb_reflectance_white_samples(FROM, TO, N));
                copy(&reflectance.cyan, color::rgb_reflectance_cyan_samples(FROM, TO, N));
                copy(&reflectance.magenta, color::rgb_reflectance_magenta_samples(FROM, TO, N));
                copy(&reflectance.yellow, color::rgb_reflectance_yellow_samples(FROM, TO, N));
                copy(&reflectance.red, color::rgb_reflectance_red_samples(FROM, TO, N));
                copy(&reflectance.green, color::rgb_reflectance_green_samples(FROM, TO, N));
                copy(&reflectance.blue, color::rgb_reflectance_blue_samples(FROM, TO, N));

                Colors& illumination = samples.illumination;
                copy(&illumination.white, color::rgb_illumination_white_samples(FROM, TO, N));
                copy(&illumination.cyan, color::rgb_illumination_cyan_samples(FROM, TO, N));
                copy(&illumination.magenta, color::rgb_illumination_magenta_samples(FROM, TO, N));
                copy(&illumination.yellow, color::rgb_illumination_yellow_samples(FROM, TO, N));
                copy(&illumination.red, color::rgb_illumination_red_samples(FROM, TO, N));
                copy(&illumination.green, color::rgb_illumination_green_samples(FROM, TO, N));
                copy(&illumination.blue, color::rgb_illumination_blue_samples(FROM, TO, N));

                return samples;
        }

        static const Samples& samples()
        {
                static const Samples samples = create_samples();
                return samples;
        }

public:
        using DataType = T;

        Spectrum()
        {
        }

        constexpr explicit Spectrum(T v) : Base(v)
        {
        }

        constexpr Spectrum(T /*red*/, T /*green*/, T /*blue*/) : Base(T(0))
        {
        }

        constexpr explicit Spectrum(const RGB8& /*c*/) : Base(T(0))
        {
        }

        template <typename F>
        [[nodiscard]] Vector<3, F> rgb() const
        {
                static_assert(std::is_floating_point_v<F>);
                return Vector<3, F>(0);
        }

        [[nodiscard]] RGB8 rgb8() const
        {
                return RGB8(0, 0, 0);
        }

        template <typename F>
        void set_rgb(const Vector<3, F>& /*rgb*/)
        {
                static_assert(std::is_floating_point_v<F>);
        }

        [[nodiscard]] T luminance() const
        {
                return 0;
        }

        [[nodiscard]] friend std::string to_string(const Spectrum& c)
        {
                return c.to_string("spectrum");
        }
};

using Color = RGB;
//using Color = Spectrum<380, 720, 64, float>;
}
