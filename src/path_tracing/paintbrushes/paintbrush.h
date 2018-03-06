/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/objects.h"

#include "com/error.h"
#include "com/thread.h"
#include "com/time.h"

class BarPaintbrush final : public Paintbrush
{
        struct Pixel
        {
                int_least16_t x, y;
                Pixel(int_least16_t px, int_least16_t py) : x(px), y(py)
                {
                }
        };

        static std::vector<Pixel> generate_pixels(int width, int height, int paint_height)
        {
                std::vector<Pixel> pixels;

                for (int y = height - 1; y >= 0; y -= paint_height)
                {
                        for (int x = 0; x < width; ++x)
                        {
                                int max_sub_y = std::min(paint_height, y + 1);
                                for (int sub_y = 0; sub_y < max_sub_y; ++sub_y)
                                {
                                        int pixel_x = x;
                                        int pixel_y = y - sub_y;

                                        ASSERT(pixel_x >= 0 && pixel_x < width);
                                        ASSERT(pixel_y >= 0 && pixel_y < height);

                                        pixels.emplace_back(pixel_x, pixel_y);
                                }
                        }
                }

                ASSERT(pixels.size() == static_cast<unsigned>(width * height));

                return pixels;
        }

        const int m_width, m_height;

        std::vector<Pixel> m_pixels;

        unsigned m_current_pixel = 0;

        long long m_pass_count = 1;
        long long m_pixel_count = 0;
        long long m_ray_count = 0;
        long long m_sample_count = 0;

        double m_previous_pass_duration = 0;
        double m_pass_start_time = -1;

        mutable SpinLock m_lock;

public:
        BarPaintbrush(int width, int height, int paint_height)
                : m_width(width), m_height(height), m_pixels(generate_pixels(width, height, paint_height))
        {
        }

        int painting_width() const noexcept override
        {
                return m_width;
        }

        int painting_height() const noexcept override
        {
                return m_height;
        }

        void first_pass() noexcept override
        {
                std::lock_guard lg(m_lock);

                m_pass_start_time = time_in_seconds();
        }

        bool next_pixel(int previous_pixel_ray_count, int previous_pixel_sample_count, int* x, int* y) noexcept override
        {
                std::lock_guard lg(m_lock);

                m_ray_count += previous_pixel_ray_count;
                m_sample_count += previous_pixel_sample_count;

                if (m_current_pixel < m_pixels.size())
                {
                        *x = m_pixels[m_current_pixel].x;
                        *y = m_pixels[m_current_pixel].y;

                        ++m_current_pixel;
                        ++m_pixel_count;

                        return true;
                }

                return false;
        }

        virtual void next_pass() noexcept override
        {
                std::lock_guard lg(m_lock);

                ASSERT(m_current_pixel == m_pixels.size());
                ASSERT(m_pass_start_time >= 0);

                double time = time_in_seconds();
                m_previous_pass_duration = time - m_pass_start_time;
                m_pass_start_time = time;

                m_current_pixel = 0;
                ++m_pass_count;
        }

        void statistics(long long* pass_count, long long* pixel_count, long long* ray_count, long long* sample_count,
                        double* previous_pass_duration) const noexcept override
        {
                std::lock_guard lg(m_lock);

                *pass_count = m_pass_count;
                *pixel_count = m_pixel_count;
                *ray_count = m_ray_count;
                *sample_count = m_sample_count;
                *previous_pass_duration = m_previous_pass_duration;
        }
};
