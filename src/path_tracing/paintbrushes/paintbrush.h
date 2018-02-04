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

#include "com/thread.h"

class BarPaintbrush final : public Paintbrush
{
        struct Pixel
        {
                int_least16_t x, y;
                Pixel(int_least16_t px, int_least16_t py) : x(px), y(py)
                {
                }
        };

        std::vector<Pixel> m_pixels;
        std::vector<int> m_map;
        std::vector<bool> m_pixels_busy;
        unsigned m_current_pixel = 0;

        int m_pass_count = 1;
        long long m_pixel_count = 0;

        unsigned m_width;

        mutable SpinLock m_lock;

public:
        BarPaintbrush(int nx, int ny, int paint_height)
        {
                m_width = nx;
                m_map.resize(nx * ny);
                for (int y = 0; y < ny; y += paint_height)
                {
                        for (int x = 0; x < nx; ++x)
                        {
                                int max_sub_y = std::min(paint_height, ny - y);
                                for (int sub_y = 0; sub_y < max_sub_y; ++sub_y)
                                {
                                        m_pixels.emplace_back(x, y + sub_y);
                                        m_map[(y + sub_y) * m_width + x] = m_pixels.size() - 1;
                                        m_pixels_busy.push_back(false);
                                }
                        }
                }
                ASSERT(m_pixels.size() == static_cast<unsigned>(nx * ny) && m_pixels.size() == m_pixels_busy.size());
        }

        void get_pixel(int* x, int* y) override
        {
                std::lock_guard lg(m_lock);

                unsigned start = m_current_pixel;

                while (m_current_pixel < m_pixels.size() && m_pixels_busy[m_current_pixel])
                {
                        ++m_current_pixel;
                }

                if (m_current_pixel == m_pixels.size())
                {
                        ++m_pass_count;

                        m_current_pixel = 0;
                        while (m_current_pixel < start && m_pixels_busy[m_current_pixel])
                        {
                                ++m_current_pixel;
                        }

                        if (m_current_pixel == start)
                        {
                                error("all pixels busy");
                        }
                }

                m_pixels_busy[m_current_pixel] = true;

                *x = m_pixels[m_current_pixel].x;
                *y = m_pixels[m_current_pixel].y;

                ++m_current_pixel;
        }

        void release_pixel(int x, int y) override
        {
                std::lock_guard lg(m_lock);

                m_pixels_busy[m_map[y * m_width + x]] = false;

                ++m_pixel_count;
        }

        void pass_and_pixel_count(int* pass_count, long long* pixel_count) const override
        {
                std::lock_guard lg(m_lock);

                *pass_count = m_pass_count;
                *pixel_count = m_pixel_count;
        }
};
