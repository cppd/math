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

#include "com/alg.h"
#include "com/error.h"
#include "com/print.h"
#include "com/thread.h"
#include "com/time.h"

template <size_t N>
class BarPaintbrush
{
        static_assert(N >= 2);

        using Pixel = std::array<int_least16_t, N>;

        // Две следующих функции проходят циклы по измерениям, кроме последнего,
        // с интервалом paint_height. По последнему измерению с интервалом 1.
        // Далее цикл по измерениям, кроме последнего, с интервалом 1 от текущего
        // значения измерения и paint_height итераций или до максимума.
        // Пример для 3 измерений
        // for(int x = 0; x < max_x; x += paint_height)
        // {
        //         for(int y = 0; y < max_y; y += paint_height)
        //         {
        //                 for(int z = 0; z < max_z; ++z)
        //                 {
        //                         for(int sub_x = x; sub_x < std::min(max_x, x + paint_height); ++sub_x)
        //                         {
        //                                 for(int sub_y = y; sub_y < std::min(max_y, y + paint_height); ++sub_y)
        //                                 {
        //                                         // pixel(sub_x, sub_y, z);
        //                                 }
        //                         }
        //                 }
        //         }
        // }
        template <int level>
        static void generate_pixels(
                Pixel& pixel,
                std::array<int, N + N - 1>& min,
                std::array<int, N + N - 1>& max,
                const std::array<int, N - 1>& inc,
                std::vector<Pixel>* pixels)
        {
                static_assert(level < N + N - 1);

                for (int i = min[level]; i < max[level]; (level < N - 1) ? (i += inc[level]) : ++i)
                {
                        if constexpr (level < N - 1)
                        {
                                min[level + N] = i;
                                max[level + N] = std::min(max[level], i + inc[level]);
                        }
                        if constexpr (level == N - 1)
                        {
                                pixel[level] = i;
                        }
                        if constexpr (level > N - 1)
                        {
                                pixel[level - N] = i;
                                ASSERT(pixel[level - N] >= min[level - N] && pixel[level - N] < max[level - N]);
                        }

                        if constexpr (level < N + N - 2)
                        {
                                generate_pixels<level + 1>(pixel, min, max, inc, pixels);
                        }
                        else
                        {
                                pixels->emplace_back(pixel);
                        }
                }
        }
        static void generate_pixels(const std::array<int, N>& sizes, int paint_height, std::vector<Pixel>* pixels)
        {
                std::array<int, N + N - 1> min;
                std::array<int, N + N - 1> max;
                std::array<int, N - 1> inc;
                Pixel pixel;

                for (unsigned i = 0; i < N; ++i)
                {
                        min[i] = 0;
                        max[i] = sizes[i];
                }

                for (unsigned i = 0; i < N - 1; ++i)
                {
                        inc[i] = paint_height;
                }

                pixels->clear();

                generate_pixels<0>(pixel, min, max, inc, pixels);

                ASSERT(static_cast<long long>(pixels->size()) == multiply_all<long long>(sizes));
        }

        std::array<int, N> m_screen_size;
        std::vector<Pixel> m_pixels;
        int m_max_pass_count;

        unsigned m_current_pixel = 0;

        long long m_pass_count = 1;
        long long m_pixel_count = 0;
        long long m_ray_count = 0;
        long long m_sample_count = 0;

        double m_previous_pass_duration = 0;
        double m_pass_start_time = -1;

        mutable SpinLock m_lock;

public:
        BarPaintbrush(const std::array<int, N>& screen_size, int paint_height, int max_pass_count)
        {
                for (unsigned i = 0; i < screen_size.size(); ++i)
                {
                        if (screen_size[i] < 1)
                        {
                                error("Paintbrush size " + to_string(i) + " is not positive (" +
                                      to_string(screen_size[i]) + ")");
                        }
                }
                if (paint_height < 1)
                {
                        error("Error paintbrush paint height " + to_string(paint_height));
                }
                if (!(max_pass_count == -1 || max_pass_count > 0))
                {
                        error("Error paintbrush max pass count " + to_string(max_pass_count));
                }

                m_screen_size = screen_size;
                m_max_pass_count = max_pass_count;

                std::reverse(m_screen_size.begin(), m_screen_size.end());

                generate_pixels(m_screen_size, paint_height, &m_pixels);

                std::reverse(m_screen_size.begin(), m_screen_size.end());

                for (Pixel& pixel : m_pixels)
                {
                        std::reverse(pixel.begin(), pixel.end());

                        if (N == 2)
                        {
                                pixel[1] = m_screen_size[1] - 1 - pixel[1];
                        }
                }
        }

        const std::array<int, N>& screen_size() const
        {
                return m_screen_size;
        }

        void first_pass()
        {
                std::lock_guard lg(m_lock);

                m_pass_start_time = time_in_seconds();
        }

        bool next_pixel(int previous_pixel_ray_count, int previous_pixel_sample_count, Pixel* pixel)
        {
                std::lock_guard lg(m_lock);

                m_ray_count += previous_pixel_ray_count;
                m_sample_count += previous_pixel_sample_count;

                if (m_current_pixel < m_pixels.size())
                {
                        *pixel = m_pixels[m_current_pixel];

                        ++m_current_pixel;
                        ++m_pixel_count;

                        return true;
                }

                return false;
        }

        bool next_pass()
        {
                std::lock_guard lg(m_lock);

                ASSERT(m_current_pixel == m_pixels.size());
                ASSERT(m_pass_start_time >= 0);

                double time = time_in_seconds();
                m_previous_pass_duration = time - m_pass_start_time;
                m_pass_start_time = time;

                m_current_pixel = 0;

                if (m_pass_count == m_max_pass_count)
                {
                        return false;
                }

                ++m_pass_count;

                return true;
        }

        void statistics(
                long long* pass_count,
                long long* pixel_count,
                long long* ray_count,
                long long* sample_count,
                double* previous_pass_duration) const
        {
                std::lock_guard lg(m_lock);

                *pass_count = m_pass_count;
                *pixel_count = m_pixel_count;
                *ray_count = m_ray_count;
                *sample_count = m_sample_count;
                *previous_pass_duration = m_previous_pass_duration;
        }
};
