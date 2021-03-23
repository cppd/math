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

#include "paintbrush.h"

#include <src/color/color.h>
#include <src/com/global_index.h>
#include <src/com/thread.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::painter
{
struct PixelInfo final
{
        Color color;
        float coverage;
};

template <std::size_t N, typename T>
class Pixel final
{
        Color m_color_sum{0};
        std::uint_least16_t m_hit_sample_sum = 0;
        std::uint_least16_t m_missed_sample_sum = 0;

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
                return static_cast<float>(m_hit_sample_sum) / (m_hit_sample_sum + m_missed_sample_sum);
        }

public:
        void add_sample(const Color& color, const Vector<N, T>& /*sample*/)
        {
                m_hit_sample_sum += 1;
                m_color_sum += color;
        }

        void add_missed()
        {
                m_missed_sample_sum += 1;
        }

        PixelInfo info() const
        {
                return {.color = compute_color(), .coverage = compute_coverage()};
        }
};

template <std::size_t N, typename T>
class Pixels final
{
        static constexpr int PANTBRUSH_WIDTH = 20;

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel<N, T>> m_pixels;

        Paintbrush<N> m_paintbrush;

        mutable SpinLock m_lock;

public:
        explicit Pixels(const std::array<int, N>& screen_size)
                : m_global_index(screen_size), m_paintbrush(screen_size, PANTBRUSH_WIDTH)
        {
                m_pixels.resize(m_global_index.count());
        }

        std::optional<std::array<int_least16_t, N>> next_pixel()
        {
                std::lock_guard lg(m_lock);

                return m_paintbrush.next_pixel();
        }

        void next_pass()
        {
                m_paintbrush.reset();
        }

        void add_sample(
                const std::array<int_least16_t, N>& pixel,
                const Vector<N, T>& sample,
                const std::optional<Color>& color)
        {
                if (color)
                {
                        m_pixels[m_global_index.compute(pixel)].add_sample(*color, sample);
                }
                else
                {
                        m_pixels[m_global_index.compute(pixel)].add_missed();
                }
        }

        PixelInfo info(const std::array<int_least16_t, N>& pixel) const
        {
                return m_pixels[m_global_index.compute(pixel)].info();
        }
};
}
