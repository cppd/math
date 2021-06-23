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
#include "rgb8.h"

#include "samples/rgb_samples.h"
#include "samples/xyz_samples.h"

#include <src/com/error.h>
#include <src/com/math.h>

#include <string>

namespace ns::color
{
enum class Type
{
        Reflectance,
        Illumination
};

template <typename T>
class RGB final : public ColorSamples<RGB<T>, 3, T>
{
        using Base = ColorSamples<RGB<T>, 3, T>;

        static constexpr Vector<3, T> make_rgb(T red, T green, T blue)
        {
                ASSERT(is_finite(red));
                ASSERT(is_finite(green));
                ASSERT(is_finite(blue));

                red = std::max(T(0), red);
                green = std::max(T(0), green);
                blue = std::max(T(0), blue);

                return Vector<3, T>(red, green, blue);
        }

public:
        using DataType = T;

        constexpr RGB()
        {
        }

        constexpr explicit RGB(std::type_identity_t<T> v) : Base(std::max(T(0), v))
        {
        }

        constexpr RGB(
                std::type_identity_t<T> red,
                std::type_identity_t<T> green,
                std::type_identity_t<T> blue,
                Type = Type::Reflectance)
                : Base(make_rgb(red, green, blue))
        {
        }

        constexpr RGB(const RGB8& c, Type = Type::Reflectance) : Base(c.linear_red(), c.linear_green(), c.linear_blue())
        {
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
                T r = std::max(T(0), Base::data()[0]);
                T g = std::max(T(0), Base::data()[1]);
                T b = std::max(T(0), Base::data()[2]);
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
        Color to_color(Type type = Type::Reflectance) const
        {
                return Color(Base::data()[0], Base::data()[1], Base::data()[2], type);
        }
};

template <typename T, std::size_t N>
class SpectrumSamples final : public ColorSamples<SpectrumSamples<T, N>, N, T>
{
        static constexpr XYZ XYZ_VERSION = XYZ_31;
        static constexpr int FROM = 380;
        static constexpr int TO = 720;

        static_assert(FROM >= XYZ_SAMPLES_MIN_WAVELENGTH);
        static_assert(FROM >= RGB_SAMPLES_MIN_WAVELENGTH);
        static_assert(TO <= XYZ_SAMPLES_MAX_WAVELENGTH);
        static_assert(TO <= RGB_SAMPLES_MAX_WAVELENGTH);
        static_assert(N > 3);

        using Base = ColorSamples<SpectrumSamples<T, N>, N, T>;

        struct Colors
        {
                Vector<N, T> white;
                Vector<N, T> cyan;
                Vector<N, T> magenta;
                Vector<N, T> yellow;
                Vector<N, T> red;
                Vector<N, T> green;
                Vector<N, T> blue;
        };

        struct Samples
        {
                Vector<N, T> x;
                Vector<N, T> y;
                Vector<N, T> z;
                Colors reflectance;
                Colors illumination;
        };

        static Samples create_samples()
        {
                const auto copy = []<typename SourceType>(Vector<N, T>* dst, const std::vector<SourceType>& src)
                {
                        static_assert(std::is_floating_point_v<SourceType>);
                        ASSERT(src.size() == N);
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                (*dst)[i] = src[i];
                        }
                };

                Samples samples;

                copy(&samples.x, cie_x_samples<XYZ_VERSION>(FROM, TO, N));
                copy(&samples.y, cie_y_samples<XYZ_VERSION>(FROM, TO, N));
                copy(&samples.z, cie_z_samples<XYZ_VERSION>(FROM, TO, N));

                {
                        Colors& c = samples.reflectance;
                        copy(&c.white, rgb_reflectance_white_samples(FROM, TO, N));
                        copy(&c.cyan, rgb_reflectance_cyan_samples(FROM, TO, N));
                        copy(&c.magenta, rgb_reflectance_magenta_samples(FROM, TO, N));
                        copy(&c.yellow, rgb_reflectance_yellow_samples(FROM, TO, N));
                        copy(&c.red, rgb_reflectance_red_samples(FROM, TO, N));
                        copy(&c.green, rgb_reflectance_green_samples(FROM, TO, N));
                        copy(&c.blue, rgb_reflectance_blue_samples(FROM, TO, N));
                }
                {
                        Colors& c = samples.illumination;
                        copy(&c.white, rgb_illumination_d65_white_samples(FROM, TO, N));
                        copy(&c.cyan, rgb_illumination_d65_cyan_samples(FROM, TO, N));
                        copy(&c.magenta, rgb_illumination_d65_magenta_samples(FROM, TO, N));
                        copy(&c.yellow, rgb_illumination_d65_yellow_samples(FROM, TO, N));
                        copy(&c.red, rgb_illumination_d65_red_samples(FROM, TO, N));
                        copy(&c.green, rgb_illumination_d65_green_samples(FROM, TO, N));
                        copy(&c.blue, rgb_illumination_d65_blue_samples(FROM, TO, N));
                }

                return samples;
        }

        static const Samples& samples()
        {
                static const Samples samples = create_samples();
                return samples;
        }

        //

        static void clamp_negative(Vector<N, T>* v)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        (*v)[i] = std::max(T(0), (*v)[i]);
                }
        }

        // Brian Smits.
        // An RGB-to-Spectrum Conversion for Reflectances.
        // Journal of Graphics Tools, 1999.
        static Vector<N, T> rgb_to_spectrum(T red, T green, T blue, const Colors& c)
        {
                ASSERT(std::isfinite(red));
                ASSERT(std::isfinite(green));
                ASSERT(std::isfinite(blue));

                red = std::max(T(0), red);
                green = std::max(T(0), green);
                blue = std::max(T(0), blue);

                Vector<N, T> spectrum(0);

                if (red <= green && red <= blue)
                {
                        spectrum.multiply_add(red, c.white);
                        if (green <= blue)
                        {
                                spectrum.multiply_add(green - red, c.cyan);
                                spectrum.multiply_add(blue - green, c.blue);
                        }
                        else
                        {
                                spectrum.multiply_add(blue - red, c.cyan);
                                spectrum.multiply_add(green - blue, c.green);
                        }
                }
                else if (green <= red && green <= blue)
                {
                        spectrum.multiply_add(green, c.white);
                        if (red <= blue)
                        {
                                spectrum.multiply_add(red - green, c.magenta);
                                spectrum.multiply_add(blue - red, c.blue);
                        }
                        else
                        {
                                spectrum.multiply_add(blue - green, c.magenta);
                                spectrum.multiply_add(red - blue, c.red);
                        }
                }
                else if (blue <= red && blue <= green)
                {
                        spectrum.multiply_add(blue, c.white);
                        if (red <= green)
                        {
                                spectrum.multiply_add(red - blue, c.yellow);
                                spectrum.multiply_add(green - red, c.green);
                        }
                        else
                        {
                                spectrum.multiply_add(green - blue, c.yellow);
                                spectrum.multiply_add(red - green, c.red);
                        }
                }
                else
                {
                        ASSERT(false);
                        return spectrum;
                }

                clamp_negative(&spectrum);

                return spectrum;
        }

        static Vector<N, T> rgb_to_spectrum(T red, T green, T blue, Type type)
        {
                const Samples& s = samples();

                switch (type)
                {
                case Type::Reflectance:
                        return rgb_to_spectrum(red, green, blue, s.reflectance);
                case Type::Illumination:
                        return rgb_to_spectrum(red, green, blue, s.illumination);
                }
                error_fatal("Unknown color type " + std::to_string(static_cast<long long>(type)));
        }

        static Vector<3, T> spectrum_to_rgb(Vector<N, T> spectrum)
        {
                const Samples& s = samples();

                clamp_negative(&spectrum);

                const T x = dot(spectrum, s.x);
                const T y = dot(spectrum, s.y);
                const T z = dot(spectrum, s.z);

                return xyz_to_linear_srgb(x, y, z);
        }

        static T spectrum_to_luminance(Vector<N, T> spectrum)
        {
                const Samples& s = samples();

                clamp_negative(&spectrum);

                return dot(spectrum, s.y);
        }

public:
        using DataType = T;

        constexpr SpectrumSamples()
        {
        }

        constexpr explicit SpectrumSamples(std::type_identity_t<T> v) : Base(std::max(T(0), v))
        {
        }

        SpectrumSamples(
                std::type_identity_t<T> red,
                std::type_identity_t<T> green,
                std::type_identity_t<T> blue,
                Type type = Type::Reflectance)
                : Base(rgb_to_spectrum(red, green, blue, type))
        {
        }

        SpectrumSamples(const RGB8& c, Type type = Type::Reflectance)
                : SpectrumSamples(c.linear_red(), c.linear_green(), c.linear_blue(), type)
        {
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
};

using Color = RGB<float>;
using Spectrum = SpectrumSamples<float, 64>;
}
