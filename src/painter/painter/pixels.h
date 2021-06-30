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

#include <src/com/global_index.h>
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

template <typename T>
class LockGuards
{
        T& m_locks;

public:
        explicit LockGuards(T& locks) noexcept : m_locks(locks)
        {
                for (auto& lock : m_locks)
                {
                        static_assert(noexcept(lock.lock()));
                        lock.lock();
                }
        }

        ~LockGuards()
        {
                for (auto& lock : m_locks)
                {
                        static_assert(noexcept(lock.unlock()));
                        lock.unlock();
                }
        }

        LockGuards(const LockGuards&) = delete;
        LockGuards(LockGuards&&) = delete;
        LockGuards& operator=(const LockGuards&) = delete;
        LockGuards& operator=(LockGuards&&) = delete;
};
}

template <std::size_t N, typename T, typename Color>
class Pixels final
{
        using PaintbrushType = std::uint_least16_t;

        static constexpr int PANTBRUSH_WIDTH = 20;

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

        const std::array<int, N> m_screen_size;
        const Region<N> m_region{m_screen_size, FILTER_RADIUS};

        const Color m_background_color;
        const Vector<3, float> m_background_color_rgb32 = m_background_color.rgb32();

        Notifier<N>* const m_notifier;

        const GlobalIndex<N, long long> m_global_index;

        std::vector<Pixel<Color>> m_pixels;
        mutable std::vector<SpinLock> m_pixel_locks;

        Paintbrush<N, PaintbrushType> m_paintbrush;
        mutable SpinLock m_paintbrush_lock;

        Vector<3, float> to_rgb(const typename Pixel<Color>::Info& info) const
        {
                if (info.alpha >= 1)
                {
                        return info.color.rgb32();
                }
                if (info.alpha <= 0)
                {
                        return m_background_color_rgb32;
                }
                const Color c = info.color + (1 - info.alpha) * m_background_color;
                return c.rgb32();
        }

public:
        Pixels(const std::array<int, N>& screen_size,
               const std::type_identity_t<Color>& background_color,
               Notifier<N>* notifier)
                : m_screen_size(screen_size),
                  m_background_color(background_color),
                  m_notifier(notifier),
                  m_global_index(screen_size),
                  m_pixels(m_global_index.count()),
                  m_pixel_locks(m_pixels.size()),
                  m_paintbrush(screen_size, PANTBRUSH_WIDTH)
        {
        }

        std::optional<std::array<int, N>> next_pixel()
        {
                namespace impl = pixels_implementation;

                return impl::to_type<int>(
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

                const auto f = [&](const std::array<int, N>& region_pixel)
                {
                        const Vector<N, T> region_pixel_center_in_point_coordinates = [&]()
                        {
                                Vector<N, T> r;
                                for (unsigned i = 0; i < N; ++i)
                                {
                                        r[i] = (region_pixel[i] - pixel[i]) + T(0.5);
                                }
                                return r;
                        }();

                        Color color_sum{0};
                        T hit_weight_sum{0};
                        T background_weight_sum{0};

                        for (std::size_t i = 0; i < points.size(); ++i)
                        {
                                const T weight = m_filter.compute(region_pixel_center_in_point_coordinates - points[i]);

                                if (colors[i])
                                {
                                        color_sum.multiply_add(weight, *colors[i]);
                                        hit_weight_sum += weight;
                                }
                                else
                                {
                                        background_weight_sum += weight;
                                }
                        }

                        const long long region_pixel_index = m_global_index.compute(region_pixel);

                        std::lock_guard lg(m_pixel_locks[region_pixel_index]);

                        Pixel<Color>& p = m_pixels[region_pixel_index];
                        p.merge(color_sum, hit_weight_sum, background_weight_sum);
                        m_notifier->pixel_set(region_pixel, to_rgb(p.info()));
                };

                m_region.region(pixel, f);
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

                impl::LockGuards lg(m_pixel_locks);

                std::byte* ptr_rgb = image_rgb->pixels.data();
                std::byte* ptr_rgba = image_rgba->pixels.data();
                for (const Pixel<Color>& pixel : m_pixels)
                {
                        const typename Pixel<Color>::Info info = pixel.info();

                        RGBA rgba;
                        rgba.rgb = info.color.rgb32();
                        rgba.alpha = info.alpha;

                        Vector<3, float> rgb;
                        if (info.alpha >= 1)
                        {
                                rgb = rgba.rgb;
                        }
                        else if (info.alpha <= 0)
                        {
                                rgb = m_background_color_rgb32;
                        }
                        else
                        {
                                const Color c = info.color + (1 - info.alpha) * m_background_color;
                                rgb = c.rgb32();
                        }

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
