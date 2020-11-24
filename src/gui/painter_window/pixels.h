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

#include <src/color/color.h>
#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/error.h>

#include <cstring>
#include <span>
#include <vector>

namespace gui::painter_window
{
class Pixels
{
        static constexpr unsigned char ALPHA_FOR_FULL_COVERAGE = 1;

        const Color m_background_color;
        std::vector<std::byte> m_pixels_bgra;
        const size_t m_slice_size;
        long long m_slice_offset;

        static std::vector<std::byte> make_initial_bgra_image(const std::vector<int>& screen_size)
        {
                constexpr Srgb8 LIGHT = Srgb8(100, 150, 200);
                constexpr Srgb8 DARK = Srgb8(0, 0, 0);

                constexpr std::array<unsigned char, 4> LIGHT_BGR = {LIGHT.blue, LIGHT.green, LIGHT.red, 0};
                constexpr std::array<unsigned char, 4> DARK_BGR = {DARK.blue, DARK.green, DARK.red, 0};

                const int count = multiply_all<long long>(screen_size)
                                  / (static_cast<long long>(screen_size[0]) * static_cast<long long>(screen_size[1]));

                std::vector<std::byte> image(4 * multiply_all<long long>(screen_size));

                size_t index = 0;
                for (int i = 0; i < count; ++i)
                {
                        for (int y = 0; y < screen_size[1]; ++y)
                        {
                                for (int x = 0; x < screen_size[0]; ++x)
                                {
                                        std::memcpy(
                                                &image[index], ((x + y) & 1) ? LIGHT_BGR.data() : DARK_BGR.data(), 4);
                                        index += 4;
                                }
                        }
                }

                ASSERT(index == image.size());

                return image;
        }

        void set(long long pixel_index, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
        {
                const std::array<unsigned char, 4> c{b, g, r, a};
                std::memcpy(&m_pixels_bgra[4 * pixel_index], c.data(), 4);
        }

public:
        Pixels(const std::vector<int>& screen_size, const Color& background_color, long long slice_pixel_index)
                : m_background_color(background_color),
                  m_pixels_bgra(make_initial_bgra_image(screen_size)),
                  m_slice_size(4ull * screen_size[0] * screen_size[1]),
                  m_slice_offset(4 * slice_pixel_index)
        {
        }

        void set(long long pixel_index, const Color& color, float coverage)
        {
                if (coverage >= 1)
                {
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(color.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(color.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(color.blue());
                        set(pixel_index, r, g, b, ALPHA_FOR_FULL_COVERAGE);
                }
                else if (coverage <= 0)
                {
                        const Color& c = m_background_color;
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set(pixel_index, r, g, b, 0);
                }
                else
                {
                        const Color& c = interpolation(m_background_color, color, coverage);
                        unsigned char r = color_conversion::linear_float_to_srgb_uint8(c.red());
                        unsigned char g = color_conversion::linear_float_to_srgb_uint8(c.green());
                        unsigned char b = color_conversion::linear_float_to_srgb_uint8(c.blue());
                        set(pixel_index, r, g, b, 0);
                }
        }

        void set_slice_offset(long long slice_pixel_index)
        {
                m_slice_offset = 4 * slice_pixel_index;
        }

        std::span<const std::byte> slice() const
        {
                return std::span(&m_pixels_bgra[m_slice_offset], m_slice_size);
        }

        const std::vector<std::byte>& pixels() const
        {
                return m_pixels_bgra;
        }
};
}
