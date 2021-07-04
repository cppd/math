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

#include "filter.h"
#include "paintbrush.h"
#include "pixel.h"
#include "region.h"

#include "../painter.h"

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/log.h>
#include <src/com/spin_lock.h>
#include <src/com/type/limit.h>
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

template <std::size_t N, typename T, typename Color>
class Filter final
{
        //radius=1.5;
        //width=radius/2.5;
        //alpha=1/(2*width*width);
        //gaussian[x_]:=Exp[-alpha*x*x];
        //gaussianFilter[x_]:=gaussian[x]-gaussian[radius];
        //max=gaussianFilter[0];
        //triangle[x_]:=If[x<0,max/radius*x+max,-max/radius*x+max];
        //Plot[{gaussianFilter[x],triangle[x]},{x,-radius,radius},PlotRange->Full]
        static constexpr T FILTER_RADIUS = 1.5;
        static constexpr T GAUSSIAN_FILTER_WIDTH = FILTER_RADIUS / 2.5;

        const GaussianFilter<T> m_filter{GAUSSIAN_FILTER_WIDTH, FILTER_RADIUS};

public:
        static T radius()
        {
                return FILTER_RADIUS;
        }

        static T contribution(const Color& sample)
        {
                return sample.luminance();
        }

        struct ColorSamples final
        {
                Color sum_color{0};
                T sum_weight{0};
                Color min_color;
                T min_contribution;
                T min_weight;
                Color max_color;
                T max_contribution;
                T max_weight;

                ColorSamples()
                {
                }
        };

        std::optional<ColorSamples> color_samples(
                const Vector<N, T>& center,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors) const
        {
                thread_local std::vector<Color> samples;
                thread_local std::vector<T> contributions;
                thread_local std::vector<T> weights;

                samples.clear();
                contributions.clear();
                weights.clear();

                T min = limits<T>::max();
                T max = limits<T>::lowest();
                std::size_t min_i = limits<std::size_t>::max();
                std::size_t max_i = limits<std::size_t>::max();

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (!colors[i])
                        {
                                continue;
                        }

                        const T weight = m_filter.compute(center - points[i]);
                        ASSERT(weight >= 0);

                        if (!(weight > 0))
                        {
                                continue;
                        }

                        samples.push_back(weight * (*colors[i]));
                        contributions.push_back(contribution(samples.back()));
                        weights.push_back(weight);

                        if (contributions.back() < min)
                        {
                                min = contributions.back();
                                min_i = samples.size() - 1;
                        }

                        if (contributions.back() > max)
                        {
                                max = contributions.back();
                                max_i = samples.size() - 1;
                        }
                }

                if (samples.empty())
                {
                        return std::nullopt;
                }

                ASSERT(min_i < samples.size());
                ASSERT(max_i < samples.size());

                std::optional<ColorSamples> r(std::in_place);

                r->min_color = samples[min_i];
                r->min_contribution = contributions[min_i];
                r->min_weight = weights[min_i];

                r->max_color = samples[max_i];
                r->max_contribution = contributions[max_i];
                r->max_weight = weights[max_i];

                if (samples.size() > 2)
                {
                        for (std::size_t i = 0; i < samples.size(); ++i)
                        {
                                if (i != min_i && i != max_i)
                                {
                                        r->sum_color += samples[i];
                                        r->sum_weight += weights[i];
                                }
                        }
                }

                return r;
        }

        struct BackgroundSamples final
        {
                T sum{0};
                T min;
                T max;

                BackgroundSamples()
                {
                }
        };

        std::optional<BackgroundSamples> background_samples(
                const Vector<N, T>& center,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors) const
        {
                thread_local std::vector<T> weights;

                weights.clear();

                T min = limits<T>::max();
                T max = limits<T>::lowest();
                std::size_t min_i = limits<std::size_t>::max();
                std::size_t max_i = limits<std::size_t>::max();

                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (colors[i])
                        {
                                continue;
                        }

                        const T weight = m_filter.compute(center - points[i]);
                        ASSERT(weight >= 0);

                        if (!(weight > 0))
                        {
                                continue;
                        }

                        weights.push_back(weight);

                        if (weight < min)
                        {
                                min = weight;
                                min_i = weights.size() - 1;
                        }

                        if (weight > max)
                        {
                                max = weight;
                                max_i = weights.size() - 1;
                        }
                }

                if (weights.empty())
                {
                        return std::nullopt;
                }

                ASSERT(min_i < weights.size());
                ASSERT(max_i < weights.size());

                std::optional<BackgroundSamples> r(std::in_place);

                r->min = weights[min_i];
                r->max = weights[max_i];

                if (weights.size() > 2)
                {
                        for (std::size_t i = 0; i < weights.size(); ++i)
                        {
                                if (i != min_i && i != max_i)
                                {
                                        r->sum += weights[i];
                                }
                        }
                }

                return r;
        }
};
}

template <std::size_t N, typename T, typename Color>
class Pixels final
{
        using PaintbrushType = std::uint_least16_t;

        static constexpr int PANTBRUSH_WIDTH = 20;

        const pixels_implementation::Filter<N, T, Color> m_filter;

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
                const Vector<3, float> color =
                        p.has_color() ? p.color(m_background, m_background_contribution).rgb32() : m_background_rgb32;
                m_notifier->pixel_set(pixel, color);
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
                        if (!color->is_finite())
                        {
                                LOG("Not finite color " + to_string(*color));
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

                struct RGBA
                {
                        Vector<3, float> rgb;
                        float alpha;
                };

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
                        const Pixel<Color>& pixel = m_pixels[i];

                        std::lock_guard lg(m_pixel_locks[i]);

                        const Vector<3, float> rgb = [&]
                        {
                                if (pixel.has_color())
                                {
                                        return pixel.color(m_background, m_background_contribution).rgb32();
                                }
                                return m_background_rgb32;
                        }();

                        const RGBA rgba = [&]
                        {
                                if (pixel.has_color_alpha())
                                {
                                        const auto& [color, alpha] = pixel.color_alpha();
                                        return RGBA{.rgb = color.rgb32(), .alpha = alpha};
                                }
                                return RGBA{.rgb = Vector<3, float>(0), .alpha = 0};
                        }();

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
