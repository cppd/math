/*
Copyright (C) 2017-2022 Topological Manifold

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
#include "pixel/filter.h"
#include "pixel/pixel.h"
#include "pixel/region.h"
#include "pixel/samples.h"

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/log.h>
#include <src/com/spinlock.h>
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

        const pixel::Filter<N, T> filter_;

        const std::array<int, N> screen_size_;
        const GlobalIndex<N, long long> global_index_{screen_size_};
        const pixel::Region<N> pixel_region_{screen_size_, filter_.integer_radius()};

        const Color background_;
        const Vector<3, float> background_rgb32_ = background_.rgb32();
        const T background_contribution_ = pixel::sample_color_contribution(background_);

        Notifier<N>* const notifier_;

        std::vector<pixel::Pixel<Color>> pixels_{static_cast<std::size_t>(global_index_.count())};
        mutable std::vector<Spinlock> pixel_locks_{pixels_.size()};

        Paintbrush<N, PaintbrushType> paintbrush_{screen_size_, PANTBRUSH_WIDTH};
        mutable std::mutex paintbrush_lock_;

        Vector<4, float> rgba_color(const pixel::Pixel<Color>& pixel) const
        {
                const auto color_alpha = pixel.color_alpha(background_contribution_);
                if (color_alpha)
                {
                        const Vector<3, float> rgb = std::get<0>(*color_alpha).rgb32();
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

        Vector<3, float> rgb_color(const pixel::Pixel<Color>& pixel) const
        {
                const auto color = pixel.color(background_, background_contribution_);
                if (color)
                {
                        const Vector<3, float> rgb = color->rgb32();
                        if (!is_finite(rgb))
                        {
                                LOG("Not finite RGB color " + to_string(rgb));
                        }
                        return rgb;
                }
                return background_rgb32_;
        }

        void add_samples(
                const std::array<int, N>& region_pixel,
                const std::array<int, N>& sample_pixel,
                const std::vector<Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors)
        {
                const Vector<N, T> center = [&]()
                {
                        Vector<N, T> r;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                r[i] = (region_pixel[i] - sample_pixel[i]) + T{0.5};
                        }
                        return r;
                }();

                thread_local std::vector<T> weights;

                filter_.compute_weights(center, points, &weights);

                const auto color_samples = pixel::make_color_samples(colors, weights);
                const auto background_samples = pixel::make_background_samples(colors, weights);

                const long long index = global_index_.compute(region_pixel);
                pixel::Pixel<Color>& p = pixels_[index];

                const std::lock_guard lg(pixel_locks_[index]);
                if (color_samples)
                {
                        p.merge(*color_samples);
                }
                if (background_samples)
                {
                        p.merge(*background_samples);
                }
                notifier_->pixel_set(region_pixel, rgb_color(p));
        }

public:
        Pixels(const std::array<int, N>& screen_size,
               const std::type_identity_t<Color>& background,
               Notifier<N>* const notifier)
                : screen_size_(screen_size),
                  background_(background.max_n(0)),
                  notifier_(notifier)
        {
                if (!background.is_finite())
                {
                        error("Not finite background " + to_string(background));
                }
                if (!is_finite(background_rgb32_))
                {
                        error("Not finite background RGB " + to_string(background_rgb32_));
                }
        }

        std::optional<std::array<int, N>> next_pixel()
        {
                return pixels_implementation::to_type<int>(
                        [&]
                        {
                                const std::lock_guard lg(paintbrush_lock_);
                                return paintbrush_.next_pixel();
                        }());
        }

        void next_pass()
        {
                const std::lock_guard lg(paintbrush_lock_);
                paintbrush_.reset();
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

                pixel_region_.traverse(
                        pixel,
                        [&](const std::array<int, N>& region_pixel)
                        {
                                add_samples(region_pixel, pixel, points, colors);
                        });
        }

        void images(image::Image<N>* const image_rgb, image::Image<N>* const image_rgba) const
        {
                constexpr std::size_t RGB_PIXEL_SIZE = 3 * sizeof(float);
                constexpr std::size_t RGBA_PIXEL_SIZE = 4 * sizeof(float);

                image_rgb->color_format = image::ColorFormat::R32G32B32;
                image_rgb->size = screen_size_;
                image_rgb->pixels.resize(RGB_PIXEL_SIZE * pixels_.size());

                image_rgba->color_format = image::ColorFormat::R32G32B32A32_PREMULTIPLIED;
                image_rgba->size = screen_size_;
                image_rgba->pixels.resize(RGBA_PIXEL_SIZE * pixels_.size());

                std::byte* ptr_rgb = image_rgb->pixels.data();
                std::byte* ptr_rgba = image_rgba->pixels.data();
                for (std::size_t i = 0; i < pixels_.size(); ++i)
                {
                        Vector<4, float> rgba;
                        Vector<3, float> rgb;

                        {
                                const pixel::Pixel<Color>& pixel = pixels_[i];
                                const std::lock_guard lg(pixel_locks_[i]);
                                rgba = rgba_color(pixel);
                                rgb = rgb_color(pixel);
                        }

                        ASSERT(rgba[3] < 1 || !is_finite(rgba) || !is_finite(rgb)
                               || (rgb[0] == rgba[0] && rgb[1] == rgba[1] && rgb[2] == rgba[2]));
                        ASSERT(rgba[3] > 0 || !is_finite(rgb) || (rgb == background_rgb32_));

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
