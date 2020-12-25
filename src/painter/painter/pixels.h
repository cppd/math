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
#include <src/com/global_index.h>

#include <array>
#include <vector>

namespace ns::painter
{
struct PixelInfo final
{
        Color color;
        float coverage;
};

template <size_t N>
class Pixels final
{
        using CounterType = std::uint_least16_t;

        class Pixel
        {
                Color m_color_sum{0};
                CounterType m_hit_sample_sum = 0;
                CounterType m_all_sample_sum = 0;

                Color compute_color() const
                {
                        if (m_hit_sample_sum > 0)
                        {
                                Color c = m_color_sum / m_hit_sample_sum;
                                for (unsigned i = 0; i < 3; ++i)
                                {
                                        if (c.data()[i] > 1)
                                        {
                                                c.data()[i] = 1;
                                        }
                                }
                                return c;
                        }
                        return Color(0);
                }

                float compute_coverage() const
                {
                        return static_cast<float>(m_hit_sample_sum) / m_all_sample_sum;
                }

        public:
                PixelInfo add(const Color& color, CounterType hit_samples, CounterType all_samples)
                {
                        m_all_sample_sum += all_samples;
                        m_hit_sample_sum += hit_samples;
                        m_color_sum += color;

                        return {.color = compute_color(), .coverage = compute_coverage()};
                }
        };

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel> m_pixels;

public:
        explicit Pixels(const std::array<int, N>& screen_size) : m_global_index(screen_size)
        {
                m_pixels.resize(m_global_index.count());
        }

        PixelInfo add(
                const std::array<int_least16_t, N>& pixel,
                const Color& color,
                CounterType hit_samples,
                CounterType all_samples)
        {
                return m_pixels[m_global_index.compute(pixel)].add(color, hit_samples, all_samples);
        }
};
}
