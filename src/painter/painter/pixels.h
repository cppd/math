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
#include "pixel.h"
#include "pixel_filter.h"
#include "region.h"

#include "../painter.h"

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/log.h>
#include <src/com/spin_lock.h>
#include <src/image/image.h>

#include <array>
#include <mutex>
#include <optional>
#include <vector>

namespace ns::painter
{
namespace pixels_implementation
{
template <typename Dst, std::size_t N, typename T>
std::optional<std::array<Dst, N>> to_type(std::optional<std::array<T, N>>&& p)
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
}

template <std::size_t N, typename T, typename Color>
class Pixels final
{
        using PaintbrushType = std::uint_least16_t;

        static constexpr int PANTBRUSH_WIDTH = 20;

        const PixelFilter<N, T, Color> m_filter;

        const std::array<int, N> m_screen_size;
        const GlobalIndex<N, long long> m_global_index{m_screen_size};
        const Region<N> m_filter_region{m_screen_size, m_filter.radius()};

        const Color m_background;
        const Vector<3, float> m_background_rgb32 = m_background.rgb32();
        const T m_background_contribution = m_filter.contribution(m_background);

        Notifier<N>* const m_notifier;

        std::vector<Pixel<Color>> m_pixels{static_cast<std::size_t>(m_global_index.count())};
        mutable std::vector<SpinLock> m_pixel_locks{m_pixels.size()};

        Paintbrush<N, PaintbrushType> m_paintbrush{m_screen_size, PANTBRUSH_WIDTH};
        mutable SpinLock m_paintbrush_lock;

        Vector<4, float> rgba_color(const Pixel<Color>& pixel) const
        {
                const auto color_alpha = pixel.color_alpha(m_background_contribution);
                if (color_alpha)
                {
                        Vector<3, float> rgb = std::get<0>(*color_alpha).rgb32();
                        Vector<4, float> rgba;
                        rgba[0] = rgb[0];
                        rgba[1] = rgb[1];
                        rgba[2] = rgb[2];
                        rgba[3] = std::get<1>(*color_alpha);
                        if (!is_finite(rgba))
                        {
                                LOG("Not finite RGBA color " + to_string(rgba));
                        }
                        return rgba;
                }
                return Vector<4, float>(0);
        }

        Vector<3, float> rgb_color(const Pixel<Color>& pixel) const
        {
                const auto color = pixel.color(m_background, m_background_contribution);
                if (color)
                {
                        Vector<3, float> rgb = color->rgb32();
                        if (!is_finite(rgb))
                        {
                                LOG("Not finite RGB color " + to_string(rgb));
                        }
                        return rgb;
                }
                return m_background_rgb32;
        }

        void add_samples(
                const std::array<int, N>& pixel,
                const std::array<int, N>& sample_pixel,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors)
        {
                const Vector<N, T> center = [&]()
                {
                        Vector<N, T> r;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                r[i] = (pixel[i] - sample_pixel[i]) + T(0.5);
                        }
                        return r;
                }();

                const auto c = m_filter.color_samples(center, points, colors);
                const auto b = m_filter.background_samples(center, points, colors);

                const long long index = m_global_index.compute(pixel);
                Pixel<Color>& p = m_pixels[index];

                std::lock_guard lg(m_pixel_locks[index]);
                if (c)
                {
                        p.merge_color(
                                c->sum_color, c->sum_weight, c->min_color, c->min_contribution, c->min_weight,
                                c->max_color, c->max_contribution, c->max_weight);
                }
                if (b)
                {
                        p.merge_background(b->sum, b->min, b->max);
                }
                m_notifier->pixel_set(pixel, rgb_color(p));
        }

public:
        Pixels(const std::array<int, N>& screen_size,
               const std::type_identity_t<Color>& background,
               Notifier<N>* notifier)
                : m_screen_size(screen_size), m_background(background.max_n(0)), m_notifier(notifier)
        {
                if (!background.is_finite())
                {
                        error("Not finite background " + to_string(background));
                }
                if (!is_finite(m_background_rgb32))
                {
                        error("Not finite background RGB " + to_string(m_background_rgb32));
                }
        }

        std::optional<std::array<int, N>> next_pixel()
        {
                return pixels_implementation::to_type<int>(
                        [&]
                        {
                                std::lock_guard lg(m_paintbrush_lock);
                                return m_paintbrush.next_pixel();
                        }());
        }

        void next_pass()
        {
                std::lock_guard lg(m_paintbrush_lock);
                m_paintbrush.reset();
        }

        void add_samples(
                const std::array<int, N>& pixel,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors)
        {
                ASSERT(points.size() == colors.size());
                ASSERT(!points.empty());

                for (const std::optional<Color>& color : colors)
                {
                        if (color && !color->is_finite())
                        {
                                LOG("Not finite sample color " + to_string(*color));
                        }
                }

                m_filter_region.traverse(
                        pixel,
                        [&](const std::array<int, N>& region_pixel)
                        {
                                add_samples(region_pixel, pixel, points, colors);
                        });
        }

        void images(image::Image<N>* image_rgb, image::Image<N>* image_rgba) const
        {
                namespace impl = pixels_implementation;

                constexpr std::size_t RGB_PIXEL_SIZE = 3 * sizeof(float);
                constexpr std::size_t RGBA_PIXEL_SIZE = 4 * sizeof(float);

                image_rgb->color_format = image::ColorFormat::R32G32B32;
                image_rgb->size = m_screen_size;
                image_rgb->pixels.resize(RGB_PIXEL_SIZE * m_pixels.size());

                image_rgba->color_format = image::ColorFormat::R32G32B32A32_PREMULTIPLIED;
                image_rgba->size = m_screen_size;
                image_rgba->pixels.resize(RGBA_PIXEL_SIZE * m_pixels.size());

                std::byte* ptr_rgb = image_rgb->pixels.data();
                std::byte* ptr_rgba = image_rgba->pixels.data();
                for (std::size_t i = 0; i < m_pixels.size(); ++i)
                {
                        Vector<4, float> rgba;
                        Vector<3, float> rgb;

                        {
                                const Pixel<Color>& pixel = m_pixels[i];
                                std::lock_guard lg(m_pixel_locks[i]);
                                rgba = rgba_color(pixel);
                                rgb = rgb_color(pixel);
                        }

                        ASSERT(rgba[3] < 1 || !is_finite(rgba) || !is_finite(rgb)
                               || (rgb[0] == rgba[0] && rgb[1] == rgba[1] && rgb[2] == rgba[2]));
                        ASSERT(rgba[3] > 0 || !is_finite(rgb) || (rgb == m_background_rgb32));

                        static_assert(sizeof(rgb) == RGB_PIXEL_SIZE);
                        std::memcpy(ptr_rgb, &rgb, RGB_PIXEL_SIZE);
                        ptr_rgb += RGB_PIXEL_SIZE;

                        static_assert(sizeof(rgba) == RGBA_PIXEL_SIZE);
                        std::memcpy(ptr_rgba, &rgba, RGBA_PIXEL_SIZE);
                        ptr_rgba += RGBA_PIXEL_SIZE;
                }
                ASSERT(ptr_rgb == image_rgb->pixels.data() + image_rgb->pixels.size());
                ASSERT(ptr_rgba == image_rgba->pixels.data() + image_rgba->pixels.size());
        }
};
}
