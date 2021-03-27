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
#include <src/image/image.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::painter
{
template <std::size_t N, typename T>
class Pixel final
{
        Color m_color_sum{0};
        Color::DataType m_hit_weight_sum{0};
        Color::DataType m_background_weight_sum{0};

public:
        void add_sample(const Color& color, const Vector<N, T>& /*point*/)
        {
                m_hit_weight_sum += 1;
                m_color_sum += color;
        }

        void add_background()
        {
                m_background_weight_sum += 1;
        }

        struct Info final
        {
                Color color;
                Color::DataType alpha;
        };

        Info info() const
        {
                Info info;
                const Color::DataType sum = m_hit_weight_sum + m_background_weight_sum;
                if (sum > 0)
                {
                        info.color = m_color_sum / sum;
                        info.alpha = 1 - m_background_weight_sum / sum;
                }
                else
                {
                        info.color = Color(0);
                        info.alpha = 0;
                }
                return info;
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

        const std::array<int, N> m_screen_size;
        Notifier<N>* const m_notifier;

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel<N, T>> m_pixels;

        Paintbrush<N, PaintbrushType> m_paintbrush;

        mutable SpinLock m_lock;

public:
        explicit Pixels(const std::array<int, N>& screen_size, Notifier<N>* notifier)
                : m_screen_size(screen_size),
                  m_notifier(notifier),
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
                                m_pixels[index].add_background();
                        }
                }
                typename Pixel<N, T>::Info info = m_pixels[index].info();
                m_notifier->pixel_set(pixel, info.color, info.alpha);
        }

        image::Image<N> image() const
        {
                image::Image<N> image;

                image.color_format = image::ColorFormat::R32G32B32A32;
                image.size = m_screen_size;
                image.pixels.resize(4 * sizeof(float) * m_pixels.size());

                std::byte* ptr = image.pixels.data();
                for (const Pixel<N, T>& pixel : m_pixels)
                {
                        typename Pixel<N, T>::Info info = pixel.info();

                        float v;

                        v = info.color.red();
                        std::memcpy(ptr, &v, sizeof(v));
                        ptr += sizeof(v);

                        v = info.color.green();
                        std::memcpy(ptr, &v, sizeof(v));
                        ptr += sizeof(v);

                        v = info.color.blue();
                        std::memcpy(ptr, &v, sizeof(v));
                        ptr += sizeof(v);

                        v = info.alpha;
                        std::memcpy(ptr, &v, sizeof(v));
                        ptr += sizeof(v);
                }
                ASSERT(ptr == image.pixels.data() + image.pixels.size());

                return image;
        }
};
}
