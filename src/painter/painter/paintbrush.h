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

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::painter
{
template <std::size_t N>
class Paintbrush final
{
        static_assert(N >= 2);

        using Pixel = std::array<int_least16_t, N>;

        // Пример для 2 измерений
        //for (int x = 0; x < screen[0]; x += paintbrush[0])
        //{
        //        for (int y = 0; y < screen[1]; y += paintbrush[1])
        //        {
        //                for (int sub_x = x; sub_x < std::min(screen[0], x + paintbrush[0]); ++sub_x)
        //                {
        //                        for (int sub_y = y; sub_y < std::min(paintbrush[1], y + paintbrush[1]); ++sub_y)
        //                        {
        //                                // pixel(sub_x, sub_y);
        //                        }
        //                }
        //        }
        //}

        template <std::size_t LEVEL>
        static void generate_pixels(
                const std::array<int, N>& screen_size,
                const std::array<int, N>& paintbrush_size,
                Pixel& pixel,
                std::array<int, N>& min,
                std::array<int, N>& max,
                std::vector<Pixel>* pixels)
        {
                static_assert(LEVEL < 2 * N);

                if constexpr (LEVEL < N)
                {
                        int i = 0;
                        while (i < screen_size[LEVEL])
                        {
                                const int next = screen_size[LEVEL] - paintbrush_size[LEVEL] >= i
                                                         ? i + paintbrush_size[LEVEL]
                                                         : screen_size[LEVEL];

                                min[LEVEL] = i;
                                max[LEVEL] = next;
                                ASSERT(min[LEVEL] < max[LEVEL]);

                                generate_pixels<LEVEL + 1>(screen_size, paintbrush_size, pixel, min, max, pixels);

                                i = next;
                        }
                }
                else
                {
                        constexpr std::size_t n = LEVEL - N;
                        static_assert(n < N);

                        for (int i = min[n]; i < max[n]; ++i)
                        {
                                pixel[n] = i;
                                ASSERT(pixel[n] >= 0 && pixel[n] < screen_size[n]);

                                if constexpr (LEVEL + 1 < 2 * N)
                                {
                                        static_assert(n < N - 1);
                                        generate_pixels<LEVEL + 1>(
                                                screen_size, paintbrush_size, pixel, min, max, pixels);
                                }
                                else
                                {
                                        static_assert(n == N - 1);
                                        pixels->emplace_back(pixel);
                                }
                        }
                }
        }

        static std::vector<Pixel> generate_pixels(
                const std::array<int, N>& screen_size,
                const std::array<int, N>& paintbrush_size)
        {
                std::array<int, N> min;
                std::array<int, N> max;

                for (std::size_t i = 0; i < N; ++i)
                {
                        min[i] = 0;
                        max[i] = screen_size[i];
                }

                std::vector<Pixel> pixels;

                Pixel pixel;
                generate_pixels<0>(screen_size, paintbrush_size, pixel, min, max, &pixels);
                ASSERT(static_cast<long long>(pixels.size()) == multiply_all<long long>(screen_size));

                return pixels;
        }

        static std::vector<Pixel> generate_pixels(std::array<int, N> screen_size, int paint_height)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (screen_size[i] < 1)
                        {
                                error("Paintbrush screen size " + to_string(screen_size) + " is not positive");
                        }
                }

                if (paint_height < 1)
                {
                        error("Paintbrush size " + to_string(paint_height) + " is not positive");
                }

                std::array<int, N> paintbrush_size;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        paintbrush_size[i] = std::min(screen_size[i], paint_height);
                }
                paintbrush_size[N - 1] = 1;

                std::reverse(screen_size.begin(), screen_size.end());
                std::vector<Pixel> pixels = generate_pixels(screen_size, paintbrush_size);
                std::reverse(screen_size.begin(), screen_size.end());

                for (Pixel& pixel : pixels)
                {
                        std::reverse(pixel.begin(), pixel.end());
                        pixel[1] = screen_size[1] - 1 - pixel[1];
                }

                return pixels;
        }

        std::vector<Pixel> m_pixels;
        unsigned long long m_current_pixel;

        void init()
        {
                m_current_pixel = 0;
        }

public:
        Paintbrush(const std::array<int, N>& screen_size, int paint_height)
                : m_pixels(generate_pixels(screen_size, paint_height))
        {
                init();
        }

        void reset()
        {
                ASSERT(m_current_pixel == m_pixels.size());

                init();
        }

        std::optional<Pixel> next_pixel()
        {
                if (m_current_pixel < m_pixels.size())
                {
                        return m_pixels[m_current_pixel++];
                }

                return std::nullopt;
        }
};
}
