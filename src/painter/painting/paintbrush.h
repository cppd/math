/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/com/type/limit.h>

#include <array>
#include <mutex>
#include <optional>
#include <vector>

namespace ns::painter::painting
{
namespace paintbrush_implementation
{
template <typename Dst, std::size_t N, typename T>
std::optional<std::array<Dst, N>> to_type(const std::optional<std::array<T, N>>& p)
{
        static_assert(!std::is_same_v<Dst, T>);
        if (p)
        {
                std::array<Dst, N> result;
                for (std::size_t i = 0; i < N; ++i)
                {
                        result[i] = (*p)[i];
                }
                return result;
        }
        return std::nullopt;
}

// Example for 2D
// for (int x = 0; x < screen[0]; x += paintbrush[0])
// {
//         for (int y = 0; y < screen[1]; y += paintbrush[1])
//         {
//                 for (int sub_x = x; sub_x < std::min(screen[0], x + paintbrush[0]); ++sub_x)
//                 {
//                         for (int sub_y = y; sub_y < std::min(paintbrush[1], y + paintbrush[1]); ++sub_y)
//                         {
//                                 // pixel(sub_x, sub_y);
//                         }
//                 }
//         }
// }

template <std::size_t LEVEL, std::size_t N, typename T>
void generate_pixels(
        const std::array<int, N>& screen_size,
        const std::array<int, N>& paintbrush_size,
        std::array<T, N>& pixel,
        std::array<int, N>& min,
        std::array<int, N>& max,
        std::vector<std::array<T, N>>* const pixels)
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
                constexpr std::size_t L_N = LEVEL - N;
                static_assert(L_N < N);

                ASSERT(min[L_N] < max[L_N] && min[L_N] >= 0 && max[L_N] <= screen_size[L_N]);

                for (int i = min[L_N]; i < max[L_N]; ++i)
                {
                        pixel[L_N] = i;

                        if constexpr (LEVEL + 1 < 2 * N)
                        {
                                static_assert(L_N < N - 1);
                                generate_pixels<LEVEL + 1>(screen_size, paintbrush_size, pixel, min, max, pixels);
                        }
                        else
                        {
                                static_assert(L_N == N - 1);
                                pixels->emplace_back(pixel);
                        }
                }
        }
}

template <typename T, std::size_t N>
std::vector<std::array<T, N>> generate_pixels(
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

        const std::size_t pixel_count = multiply_all<long long>(screen_size);

        std::vector<std::array<T, N>> pixels;
        pixels.reserve(pixel_count);

        std::array<T, N> pixel;
        generate_pixels<0>(screen_size, paintbrush_size, pixel, min, max, &pixels);
        ASSERT(pixels.size() == pixel_count);

        return pixels;
}

template <typename T, std::size_t N>
std::vector<std::array<T, N>> generate_pixels(std::array<int, N> screen_size, const int paint_height)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (screen_size[i] < 1)
                {
                        error("Paintbrush screen size " + to_string(screen_size) + " is not positive");
                }

                const unsigned max_coordinate = screen_size[i] - 1;
                if (max_coordinate > Limits<T>::max())
                {
                        error("Paintbrush screen max coordinate " + to_string(max_coordinate) + " (screen size "
                              + to_string(screen_size) + ")" + " is greater than the largest value "
                              + to_string(Limits<T>::max()) + " of pixel coordinates");
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
        std::vector<std::array<T, N>> pixels = generate_pixels<T>(screen_size, paintbrush_size);
        std::reverse(screen_size.begin(), screen_size.end());

        for (std::array<T, N>& pixel : pixels)
        {
                std::reverse(pixel.begin(), pixel.end());
                pixel[1] = screen_size[1] - 1 - pixel[1];
        }

        return pixels;
}
}

template <std::size_t N>
class Paintbrush final
{
        using T = std::uint_least16_t;

        static_assert(N >= 2);

        static_assert(std::is_integral_v<T>);
        static_assert(std::is_unsigned_v<T>);

        std::vector<std::array<T, N>> pixels_;
        unsigned long long current_pixel_;
        mutable std::mutex lock_;

        void init()
        {
                current_pixel_ = 0;
        }

public:
        Paintbrush(const std::array<int, N>& screen_size, const int paint_height)
                : pixels_(paintbrush_implementation::generate_pixels<T>(screen_size, paint_height))
        {
                init();
        }

        void next_pass()
        {
                const std::lock_guard lg(lock_);

                ASSERT(current_pixel_ == pixels_.size());
                init();
        }

        std::optional<std::array<int, N>> next_pixel()
        {
                const auto res = [this]() -> std::optional<std::array<T, N>>
                {
                        const std::lock_guard lg(lock_);

                        if (current_pixel_ < pixels_.size())
                        {
                                return pixels_[current_pixel_++];
                        }
                        return std::nullopt;
                }();

                return paintbrush_implementation::to_type<int>(res);
        }
};
}
