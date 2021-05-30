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
#include "rgb_samples.h"
#include "xyz_rgb.h"
#include "xyz_samples.h"

#include <src/com/error.h>

namespace ns
{
namespace color
{
template <typename T>
class RGB final : public ColorSamples<RGB<T>, 3, T>
{
        using Base = ColorSamples<RGB<T>, 3, T>;

public:
        using DataType = T;

        RGB()
        {
        }

        constexpr explicit RGB(std::type_identity_t<T> v) : Base(v)
        {
        }

        constexpr RGB(std::type_identity_t<T> red, std::type_identity_t<T> green, std::type_identity_t<T> blue)
                : Base(red, green, blue)
        {
        }

        constexpr RGB(const RGB8& c) : Base(c.linear_red(), c.linear_green(), c.linear_blue())
        {
        }

        template <typename F>
        [[nodiscard]] std::enable_if_t<std::is_same_v<F, T>, const Vector<3, T>&> rgb() const
        {
                return Base::m_data;
        }

        template <typename F>
        [[nodiscard]] std::enable_if_t<!std::is_same_v<F, T>, Vector<3, F>> rgb() const
        {
                static_assert(std::is_floating_point_v<F>);
                return to_vector<F>(Base::m_data);
        }

        [[nodiscard]] RGB8 rgb8() const
        {
                return make_rgb8(Base::m_data);
        }

        template <typename F>
        void set_rgb(const Vector<3, F>& rgb)
        {
                static_assert(std::is_floating_point_v<F>);
                Base::m_data = to_vector<F>(rgb);
        }

        [[nodiscard]] T luminance() const
        {
                return linear_float_to_linear_luminance(Base::m_data[0], Base::m_data[1], Base::m_data[2]);
        }

        [[nodiscard]] friend std::string to_string(const RGB& c)
        {
                return c.to_string("rgb");
        }
};

template <int FROM, int TO, std::size_t N, typename T>
class Spectrum final : public ColorSamples<Spectrum<FROM, TO, N, T>, N, T>
{
        static_assert(FROM >= XYZ_SAMPLES_MIN_WAVELENGTH);
        static_assert(TO <= XYZ_SAMPLES_MAX_WAVELENGTH);
        static_assert(N > 3);

        using Base = ColorSamples<Spectrum<FROM, TO, N, T>, N, T>;

        static constexpr XYZ XYZ_VERSION = XYZ_31;

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
                const auto copy = [](Vector<N, T>* dst, const std::vector<T>& src)
                {
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
                        (*v)[i] = std::max<T>(0, (*v)[i]);
                }
        }

        // Brian Smits.
        // An RGB-to-Spectrum Conversion for Reflectances.
        // Journal of Graphics Tools, 1999.
        static Vector<N, T> rgb_to_spectrum(T red, T green, T blue, const Colors& c)
        {
                ASSERT(is_finite(red));
                ASSERT(is_finite(green));
                ASSERT(is_finite(blue));

                red = std::max<T>(0, red);
                green = std::max<T>(0, green);
                blue = std::max<T>(0, blue);

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

        static Vector<N, T> rgb_to_spectrum(T red, T green, T blue)
        {
                const Samples& s = samples();

                return rgb_to_spectrum(red, green, blue, s.reflectance);
        }

        static Vector<3, T> spectrum_to_rgb(Vector<N, T> spectrum)
        {
                const Samples& s = samples();

                clamp_negative(&spectrum);

                const T x = dot(spectrum, s.x);
                const T y = dot(spectrum, s.y);
                const T z = dot(spectrum, s.z);

                Vector<3, T> rgb = xyz_to_linear_srgb<XYZ_VERSION>(x, y, z);
                rgb[0] = std::clamp<T>(rgb[0], 0, 1);
                rgb[1] = std::clamp<T>(rgb[1], 0, 1);
                rgb[2] = std::clamp<T>(rgb[2], 0, 1);
                return rgb;
        }

        static T spectrum_to_luminance(Vector<N, T> spectrum)
        {
                const Samples& s = samples();

                clamp_negative(&spectrum);

                return dot(spectrum, s.y);
        }

        //

        template <typename F>
        explicit Spectrum(const Vector<3, F>& rgb) : Spectrum(rgb[0], rgb[1], rgb[2])
        {
        }

public:
        using DataType = T;

        Spectrum()
        {
        }

        constexpr explicit Spectrum(std::type_identity_t<T> v) : Base(v)
        {
        }

        Spectrum(std::type_identity_t<T> red, std::type_identity_t<T> green, std::type_identity_t<T> blue)
                : Base(rgb_to_spectrum(red, green, blue))
        {
        }

        template <typename F>
        Spectrum(const RGB<F>& rgb) : Spectrum(rgb.template rgb<F>())
        {
                static_assert(std::is_floating_point_v<F>);
        }

        Spectrum(const RGB8& c) : Spectrum(c.linear_red(), c.linear_green(), c.linear_blue())
        {
        }

        template <typename F>
        [[nodiscard]] Vector<3, F> rgb() const
        {
                static_assert(std::is_floating_point_v<F>);
                return to_vector<F>(spectrum_to_rgb(Base::m_data));
        }

        [[nodiscard]] RGB8 rgb8() const
        {
                return make_rgb8(rgb<float>());
        }

        template <typename F>
        void set_rgb(const Vector<3, F>& rgb)
        {
                static_assert(std::is_floating_point_v<F>);
                Base::m_data = rgb_to_spectrum(rgb[0], rgb[1], rgb[2]);
        }

        [[nodiscard]] T luminance() const
        {
                return spectrum_to_luminance(Base::m_data);
        }

        [[nodiscard]] friend std::string to_string(const Spectrum& c)
        {
                return c.to_string("spectrum");
        }
};
}

using RGB = color::RGB<float>;
using Spectrum = color::Spectrum<380, 720, 64, float>;

using Color = RGB;
//using Color = Spectrum;
}
