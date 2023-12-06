/*
Copyright (C) 2017-2023 Topological Manifold

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
#include "rgb8.h"
#include "samples.h"

#include "samples/rgb_samples.h"
#include "samples/xyz_samples.h"

#include <src/com/error.h>
#include <src/com/math.h>

#include <algorithm>
#include <string>
#include <type_traits>

namespace ns::color
{
template <typename T>
class RGB final : public Samples<RGB<T>, 3, T>
{
        using Base = Samples<RGB<T>, 3, T>;

        static constexpr Vector<3, T> make_rgb(T red, T green, T blue)
        {
                ASSERT(is_finite(red));
                ASSERT(is_finite(green));
                ASSERT(is_finite(blue));

                red = std::max(T{0}, red);
                green = std::max(T{0}, green);
                blue = std::max(T{0}, blue);

                return Vector<3, T>(red, green, blue);
        }

public:
        using DataType = T;

        constexpr RGB()
        {
        }

        constexpr explicit RGB(const std::type_identity_t<T> v)
                : Base(std::max(T{0}, v))
        {
                ASSERT(is_finite(v));
        }

        constexpr RGB(
                const std::type_identity_t<T> red,
                const std::type_identity_t<T> green,
                const std::type_identity_t<T> blue)
                : Base(make_rgb(red, green, blue))
        {
        }

        constexpr explicit RGB(const RGB8 c)
                : Base(c.linear_red(), c.linear_green(), c.linear_blue())
        {
        }

        [[nodiscard]] static RGB illuminant(const T red, const T green, const T blue)
        {
                return RGB(red, green, blue);
        }

        [[nodiscard]] static RGB illuminant(const RGB8 c)
        {
                return RGB(c);
        }

        [[nodiscard]] constexpr Vector<3, float> rgb32() const
        {
                Vector<3, float> rgb = to_vector<float>(Base::data());
                rgb[0] = std::max(0.0f, rgb[0]);
                rgb[1] = std::max(0.0f, rgb[1]);
                rgb[2] = std::max(0.0f, rgb[2]);
                return rgb;
        }

        [[nodiscard]] constexpr T luminance() const
        {
                const T r = std::max(T{0}, Base::data()[0]);
                const T g = std::max(T{0}, Base::data()[1]);
                const T b = std::max(T{0}, Base::data()[2]);
                return linear_float_to_linear_luminance(r, g, b);
        }

        [[nodiscard]] static const char* name()
        {
                return "RGB";
        }

        [[nodiscard]] friend std::string to_string(const RGB& c)
        {
                return c.to_string("rgb");
        }

        template <typename Color>
        [[nodiscard]] friend Color to_color(const RGB& c)
        {
                return Color(c.data()[0], c.data()[1], c.data()[2]);
        }

        template <typename Color>
        [[nodiscard]] friend Color to_illuminant(const RGB& c)
        {
                return Color::illuminant(c.data()[0], c.data()[1], c.data()[2]);
        }
};

template <typename T, std::size_t N>
class SpectrumSamples final : public Samples<SpectrumSamples<T, N>, N, T>
{
        static constexpr XYZ XYZ_TYPE = XYZ_31;
        static constexpr int FROM = 380;
        static constexpr int TO = 720;

        static_assert(FROM >= XYZ_SAMPLES_MIN_WAVELENGTH);
        static_assert(FROM >= RGB_SAMPLES_MIN_WAVELENGTH);
        static_assert(TO <= XYZ_SAMPLES_MAX_WAVELENGTH);
        static_assert(TO <= RGB_SAMPLES_MAX_WAVELENGTH);
        static_assert(N > 3);

        using Base = Samples<SpectrumSamples<T, N>, N, T>;

        struct Colors final
        {
                Vector<N, T> white;
                Vector<N, T> cyan;
                Vector<N, T> magenta;
                Vector<N, T> yellow;
                Vector<N, T> red;
                Vector<N, T> green;
                Vector<N, T> blue;
        };

        struct Functions final
        {
                Vector<N, T> x;
                Vector<N, T> y;
                Vector<N, T> z;
                Colors reflectance;
                Colors illumination;
        };

        static Functions create_functions()
        {
                const auto copy = []<typename SourceType>(Vector<N, T>* const dst, const std::vector<SourceType>& src)
                {
                        static_assert(std::is_floating_point_v<SourceType>);
                        ASSERT(src.size() == N);
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                (*dst)[i] = src[i];
                        }
                };

                Functions functions;

                copy(&functions.x, cie_x_samples<XYZ_TYPE>(FROM, TO, N));
                copy(&functions.y, cie_y_samples<XYZ_TYPE>(FROM, TO, N));
                copy(&functions.z, cie_z_samples<XYZ_TYPE>(FROM, TO, N));

                {
                        Colors& c = functions.reflectance;
                        copy(&c.white, rgb_reflectance_white_samples(FROM, TO, N));
                        copy(&c.cyan, rgb_reflectance_cyan_samples(FROM, TO, N));
                        copy(&c.magenta, rgb_reflectance_magenta_samples(FROM, TO, N));
                        copy(&c.yellow, rgb_reflectance_yellow_samples(FROM, TO, N));
                        copy(&c.red, rgb_reflectance_red_samples(FROM, TO, N));
                        copy(&c.green, rgb_reflectance_green_samples(FROM, TO, N));
                        copy(&c.blue, rgb_reflectance_blue_samples(FROM, TO, N));
                }
                {
                        Colors& c = functions.illumination;
                        copy(&c.white, rgb_illumination_d65_white_samples(FROM, TO, N));
                        copy(&c.cyan, rgb_illumination_d65_cyan_samples(FROM, TO, N));
                        copy(&c.magenta, rgb_illumination_d65_magenta_samples(FROM, TO, N));
                        copy(&c.yellow, rgb_illumination_d65_yellow_samples(FROM, TO, N));
                        copy(&c.red, rgb_illumination_d65_red_samples(FROM, TO, N));
                        copy(&c.green, rgb_illumination_d65_green_samples(FROM, TO, N));
                        copy(&c.blue, rgb_illumination_d65_blue_samples(FROM, TO, N));
                }

                return functions;
        }

        static const Functions& functions()
        {
                static const Functions functions = create_functions();
                return functions;
        }

        // Brian Smits.
        // An RGB-to-Spectrum Conversion for Reflectances.
        // Journal of Graphics Tools, 1999.

        static void rgb_to_spectrum_red(
                const T red,
                const T green,
                const T blue,
                const Colors& c,
                Vector<N, T>* const spectrum)
        {
                spectrum->multiply_add(red, c.white);
                if (green <= blue)
                {
                        spectrum->multiply_add(green - red, c.cyan);
                        spectrum->multiply_add(blue - green, c.blue);
                }
                else
                {
                        spectrum->multiply_add(blue - red, c.cyan);
                        spectrum->multiply_add(green - blue, c.green);
                }
        }

        static void rgb_to_spectrum_green(
                const T red,
                const T green,
                const T blue,
                const Colors& c,
                Vector<N, T>* const spectrum)
        {
                spectrum->multiply_add(green, c.white);
                if (red <= blue)
                {
                        spectrum->multiply_add(red - green, c.magenta);
                        spectrum->multiply_add(blue - red, c.blue);
                }
                else
                {
                        spectrum->multiply_add(blue - green, c.magenta);
                        spectrum->multiply_add(red - blue, c.red);
                }
        }

        static void rgb_to_spectrum_blue(
                const T red,
                const T green,
                const T blue,
                const Colors& c,
                Vector<N, T>* const spectrum)
        {
                spectrum->multiply_add(blue, c.white);
                if (red <= green)
                {
                        spectrum->multiply_add(red - blue, c.yellow);
                        spectrum->multiply_add(green - red, c.green);
                }
                else
                {
                        spectrum->multiply_add(green - blue, c.yellow);
                        spectrum->multiply_add(red - green, c.red);
                }
        }

        static Vector<N, T> rgb_to_spectrum(T red, T green, T blue, const Colors& c)
        {
                ASSERT(std::isfinite(red));
                ASSERT(std::isfinite(green));
                ASSERT(std::isfinite(blue));

                red = std::max(T{0}, red);
                green = std::max(T{0}, green);
                blue = std::max(T{0}, blue);

                Vector<N, T> spectrum(0);

                if (red <= green && red <= blue)
                {
                        rgb_to_spectrum_red(red, green, blue, c, &spectrum);
                }
                else if (green <= red && green <= blue)
                {
                        rgb_to_spectrum_green(red, green, blue, c, &spectrum);
                }
                else if (blue <= red && blue <= green)
                {
                        rgb_to_spectrum_blue(red, green, blue, c, &spectrum);
                }
                else
                {
                        ASSERT(false);
                        return spectrum;
                }

                return spectrum.max_n(0);
        }

        static Vector<3, T> spectrum_to_rgb(const Vector<N, T>& spectrum)
        {
                const Functions& f = functions();

                const Vector<N, T> s = spectrum.max_n(0);

                const T x = dot(s, f.x);
                const T y = dot(s, f.y);
                const T z = dot(s, f.z);

                return xyz_to_linear_srgb(x, y, z);
        }

        static T spectrum_to_luminance(const Vector<N, T>& spectrum)
        {
                return dot(spectrum.max_n(0), functions().y);
        }

public:
        using DataType = T;

        static constexpr int WAVELENGTH_MIN = FROM;
        static constexpr int WAVELENGTH_MAX = TO;
        static constexpr std::size_t SAMPLE_COUNT = N;

        constexpr SpectrumSamples()
        {
        }

        constexpr explicit SpectrumSamples(const std::type_identity_t<T> v)
                : Base(std::max(T{0}, v))
        {
                ASSERT(is_finite(v));
        }

        constexpr explicit SpectrumSamples(const Vector<std::size_t{N}, std::type_identity_t<T>>& samples)
                : Base(samples.max_n(0))
        {
                ASSERT(is_finite(samples));
        }

        SpectrumSamples(
                const std::type_identity_t<T> red,
                const std::type_identity_t<T> green,
                const std::type_identity_t<T> blue)
                : Base(rgb_to_spectrum(red, green, blue, functions().reflectance))
        {
        }

        explicit SpectrumSamples(const RGB8 c)
                : SpectrumSamples(c.linear_red(), c.linear_green(), c.linear_blue())
        {
        }

        [[nodiscard]] static SpectrumSamples illuminant(const T red, const T green, const T blue)
        {
                return SpectrumSamples(rgb_to_spectrum(red, green, blue, functions().illumination));
        }

        [[nodiscard]] static SpectrumSamples illuminant(const RGB8 c)
        {
                return illuminant(c.linear_red(), c.linear_green(), c.linear_blue());
        }

        [[nodiscard]] Vector<3, float> rgb32() const
        {
                Vector<3, float> rgb = to_vector<float>(spectrum_to_rgb(Base::data()));
                rgb[0] = std::max(0.0f, rgb[0]);
                rgb[1] = std::max(0.0f, rgb[1]);
                rgb[2] = std::max(0.0f, rgb[2]);
                return rgb;
        }

        [[nodiscard]] T luminance() const
        {
                return spectrum_to_luminance(Base::data());
        }

        [[nodiscard]] static const char* name()
        {
                return "Spectrum";
        }

        [[nodiscard]] friend std::string to_string(const SpectrumSamples& c)
        {
                return c.to_string("spectrum");
        }

        template <typename Color>
                requires std::is_same_v<Color, RGB<typename Color::DataType>>
        [[nodiscard]] friend Color to_color(const SpectrumSamples& c)
        {
                const Vector<3, T> rgb = spectrum_to_rgb(c.data());
                return Color(rgb[0], rgb[1], rgb[2]);
        }

        template <typename Color>
                requires std::is_same_v<Color, SpectrumSamples>
        [[nodiscard]] friend const Color& to_color(const SpectrumSamples& c)
        {
                return c;
        }
};

using Color = RGB<float>;
using Spectrum = SpectrumSamples<float, 64>;
}
