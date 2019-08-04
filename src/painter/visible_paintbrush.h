/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "objects.h"

#include "painter/paintbrushes/paintbrush.h"

template <size_t N>
class VisibleBarPaintbrush final : public Paintbrush<N>
{
        BarPaintbrush<N> m_paintbrush;

public:
        VisibleBarPaintbrush(const std::array<int, N>& screen_size, int paint_height, int max_pass_count)
                : m_paintbrush(screen_size, paint_height, max_pass_count)
        {
        }

        const std::array<int, N>& screen_size() const override
        {
                return m_paintbrush.screen_size();
        }

        void first_pass() override
        {
                m_paintbrush.first_pass();
        }

        bool next_pixel(int previous_pixel_ray_count, int previous_pixel_sample_count,
                        std::array<int_least16_t, N>* pixel) override
        {
                return m_paintbrush.next_pixel(previous_pixel_ray_count, previous_pixel_sample_count, pixel);
        }

        bool next_pass() override
        {
                return m_paintbrush.next_pass();
        }

        void statistics(long long* pass_count, long long* pixel_count, long long* ray_count, long long* sample_count,
                        double* previous_pass_duration) const override
        {
                m_paintbrush.statistics(pass_count, pixel_count, ray_count, sample_count, previous_pass_duration);
        }
};
