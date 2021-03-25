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

#include "../painter.h"

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
        void add_sample(const Color& color, const Vector<N, T>& /*point*/)
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
        using PaintbrushType = std::uint_least16_t;

        static std::optional<std::array<int, N>> to_int(std::optional<std::array<PaintbrushType, N>>&& p)
        {
                static_assert(!std::is_same_v<int, PaintbrushType>);
                if (p)
                {
                        std::array<int, N> result;
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                result[i] = (*p)[i];
                        }
                        return result;
                }
                return std::nullopt;
        }

        Notifier<N>* const m_notifier;

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel<N, T>> m_pixels;

        Paintbrush<N, PaintbrushType> m_paintbrush;

        mutable SpinLock m_lock;

public:
        explicit Pixels(const std::array<int, N>& screen_size, Notifier<N>* notifier)
                : m_notifier(notifier),
                  m_global_index(screen_size),
                  m_pixels(m_global_index.count()),
                  m_paintbrush(screen_size, PANTBRUSH_WIDTH)
        {
        }

        std::optional<std::array<int, N>> next_pixel()
        {
                return to_int(
                        [&]
                        {
                                std::lock_guard lg(m_lock);
                                return m_paintbrush.next_pixel();
                        }());
        }

        void next_pass()
        {
                m_paintbrush.reset();
        }

        void add_samples(
                const std::array<int, N>& pixel,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors)
        {
                ASSERT(points.size() == colors.size());

                const long long index = m_global_index.compute(pixel);
                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (colors[i])
                        {
                                m_pixels[index].add_sample(*colors[i], points[i]);
                        }
                        else
                        {
                                m_pixels[index].add_missed();
                        }
                }
                PixelInfo info = m_pixels[index].info();
                m_notifier->pixel_set(pixel, info.color, info.coverage);
        }
};
}
