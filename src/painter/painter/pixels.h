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

#include "../painter.h"

#include <src/color/color.h>
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
template <std::size_t N>
std::array<int, N> max_values_for_size(const std::array<int, N>& size)
{
        std::array<int, N> max;
        for (std::size_t i = 0; i < N; ++i)
        {
                max[i] = size[i] - 1;
        }
        return max;
}

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

// ceil(radius - 0.5)
template <typename T>
constexpr int integer_radius(T radius)
{
        static_assert(std::is_floating_point_v<T>);

        const T t = std::max(T(0), radius - T(0.5));
        const int t_int = t;
        return (t == t_int) ? t_int : t_int + 1;
}
static_assert(integer_radius(0.5) == 0);
static_assert(integer_radius(1.0) == 1);
static_assert(integer_radius(1.4) == 1);
static_assert(integer_radius(1.5) == 1);
static_assert(integer_radius(1.6) == 2);

//

class Pixel final
{
        Color m_color_sum{0};
        Color::DataType m_hit_weight_sum{0};
        Color::DataType m_background_weight_sum{0};

public:
        void merge(const Color& color_sum, Color::DataType hit_weight_sum, Color::DataType background_weight_sum)
        {
                m_color_sum += color_sum;
                m_hit_weight_sum += hit_weight_sum;
                m_background_weight_sum += background_weight_sum;
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

template <std::size_t N, typename T>
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
        const std::array<int, N> m_screen_max = pixels_implementation::max_values_for_size(m_screen_size);

        const Color m_background_color;
        const Vector<3, float> m_background_color_rgb32 = m_background_color.rgb<float>();

        Notifier<N>* const m_notifier;

        const GlobalIndex<N, long long> m_global_index;

        std::vector<pixels_implementation::Pixel> m_pixels;
        mutable std::vector<SpinLock> m_pixel_locks;

        Paintbrush<N, PaintbrushType> m_paintbrush;
        mutable SpinLock m_paintbrush_lock;

        template <std::size_t I, typename F>
        void region(const std::array<int, N>& min, const std::array<int, N>& max, std::array<int, N>& p, const F& f)
        {
                for (int i = min[I]; i <= max[I]; ++i)
                {
                        p[I] = i;
                        if constexpr (I + 1 < N)
                        {
                                region<I + 1>(min, max, p, f);
                        }
                        else
                        {
                                f(p);
                        }
                }
        }

        template <typename F>
        void region(const std::array<int, N>& pixel, const F& f)
        {
                static constexpr int FILTER_INTEGER_RADIUS = pixels_implementation::integer_radius(FILTER_RADIUS);

                std::array<int, N> min;
                std::array<int, N> max;
                for (std::size_t i = 0; i < N; ++i)
                {
                        min[i] = std::max(0, pixel[i] - FILTER_INTEGER_RADIUS);
                        max[i] = std::min(m_screen_max[i], pixel[i] + FILTER_INTEGER_RADIUS);
                }
                std::array<int, N> p;
                region<0>(min, max, p, f);
        }

        Vector<3, float> to_rgb(const pixels_implementation::Pixel::Info& info) const
        {
                Vector<3, float> rgb;
                if (info.alpha >= 1)
                {
                        return info.color.rgb<float>();
                }
                else if (info.alpha <= 0)
                {
                        return m_background_color_rgb32;
                }
                else
                {
                        const Color c = info.color + (1 - info.alpha) * m_background_color;
                        return c.rgb<float>();
                }
        }

public:
        Pixels(const std::array<int, N>& screen_size, const Color& background_color, Notifier<N>* notifier)
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
                namespace impl = pixels_implementation;

                ASSERT(points.size() == colors.size());

                region(pixel,
                       [&](const std::array<int, N>& region_pixel)
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
                               Color::DataType hit_weight_sum{0};
                               Color::DataType background_weight_sum{0};

                               for (std::size_t i = 0; i < points.size(); ++i)
                               {
                                       const Color::DataType weight =
                                               m_filter.compute(region_pixel_center_in_point_coordinates - points[i]);

                                       if (colors[i])
                                       {
                                               color_sum += weight * (*colors[i]);
                                               hit_weight_sum += weight;
                                       }
                                       else
                                       {
                                               background_weight_sum += weight;
                                       }
                               }

                               const long long region_pixel_index = m_global_index.compute(region_pixel);

                               std::lock_guard lg(m_pixel_locks[region_pixel_index]);

                               impl::Pixel& p = m_pixels[region_pixel_index];
                               p.merge(color_sum, hit_weight_sum, background_weight_sum);
                               m_notifier->pixel_set(region_pixel, to_rgb(p.info()));
                       });
        }

        image::Image<N> image() const
        {
                namespace impl = pixels_implementation;

                constexpr std::size_t PIXEL_SIZE = 4 * sizeof(float);

                struct ImagePixel
                {
                        Vector<3, float> rgb;
                        float alpha;
                };

                image::Image<N> image;

                image.color_format = image::ColorFormat::R32G32B32A32_PREMULTIPLIED;
                image.size = m_screen_size;
                image.pixels.resize(PIXEL_SIZE * m_pixels.size());

                impl::LockGuards lg(m_pixel_locks);

                std::byte* ptr = image.pixels.data();
                for (const impl::Pixel& pixel : m_pixels)
                {
                        const impl::Pixel::Info info = pixel.info();

                        ImagePixel rgba;
                        rgba.rgb = info.color.rgb<float>();
                        rgba.alpha = info.alpha;

                        static_assert(sizeof(rgba) == PIXEL_SIZE);
                        std::memcpy(ptr, &rgba, PIXEL_SIZE);
                        ptr += PIXEL_SIZE;
                }
                ASSERT(ptr == image.pixels.data() + image.pixels.size());

                return image;
        }
};
}
