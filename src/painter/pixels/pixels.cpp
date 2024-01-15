/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "pixels.h"

#include "pixel.h"

#include "samples/create.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/numerical/vector.h>
#include <src/painter/painter.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

namespace ns::painter::pixels
{
namespace
{
template <typename T, std::size_t N>
[[nodiscard]] Vector<N, T> region_pixel_center(
        const std::array<int, N>& region_pixel,
        const std::array<int, N>& sample_pixel)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = (region_pixel[i] - sample_pixel[i]) + T{0.5};
        }
        return res;
}
}

template <std::size_t N, typename T, typename Color>
Pixels<N, T, Color>::Pixels(
        const std::array<int, N>& screen_size,
        const std::type_identity_t<Color>& background,
        Notifier<N>* const notifier)
        : screen_size_(screen_size),
          background_(background.max_n(0)),
          notifier_(notifier)
{
}

template <std::size_t N, typename T, typename Color>
void Pixels<N, T, Color>::add_samples(
        const std::array<int, N>& region_pixel,
        const std::array<int, N>& sample_pixel,
        const std::vector<Vector<N, T>>& points,
        const std::vector<std::optional<Color>>& colors)
{
        thread_local std::vector<T> weights;

        filter_.compute_weights(region_pixel_center<T>(region_pixel, sample_pixel), points, &weights);

        const auto color_samples = samples::create_color_samples<FILTER_SAMPLE_COUNT>(colors, weights);
        const auto background_samples = samples::create_background_samples<FILTER_SAMPLE_COUNT>(colors, weights);

        const long long index = global_index_.compute(region_pixel);
        Pixel<FILTER_SAMPLE_COUNT, Color>& pixel = pixels_[index];

        const std::lock_guard lg(pixel_locks_[index]);
        if (color_samples)
        {
                pixel.merge(*color_samples);
        }
        if (background_samples)
        {
                pixel.merge(*background_samples);
        }
        notifier_->pixel_set(region_pixel, pixel.color_rgb(background_));
}

template <std::size_t N, typename T, typename Color>
void Pixels<N, T, Color>::add_samples(
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

template <std::size_t N, typename T, typename Color>
void Pixels<N, T, Color>::images(image::Image<N>* const image_rgb, image::Image<N>* const image_rgba) const
{
        constexpr std::size_t RGB_PIXEL_SIZE = 3 * sizeof(float);
        constexpr std::size_t RGBA_PIXEL_SIZE = 4 * sizeof(float);

        image_rgb->color_format = image::ColorFormat::R32G32B32;
        image_rgb->size = screen_size_;
        image_rgb->pixels.resize(RGB_PIXEL_SIZE * pixels_.size());

        image_rgba->color_format = image::ColorFormat::R32G32B32A32_PREMULTIPLIED;
        image_rgba->size = screen_size_;
        image_rgba->pixels.resize(RGBA_PIXEL_SIZE * pixels_.size());

        Vector<3, float> rgb;
        Vector<4, float> rgba;

        static_assert(sizeof(rgb) == RGB_PIXEL_SIZE);
        static_assert(sizeof(rgba) == RGBA_PIXEL_SIZE);

        std::byte* ptr_rgb = image_rgb->pixels.data();
        std::byte* ptr_rgba = image_rgba->pixels.data();
        for (std::size_t i = 0; i < pixels_.size(); ++i)
        {
                {
                        const Pixel<FILTER_SAMPLE_COUNT, Color>& pixel = pixels_[i];
                        const std::lock_guard lg(pixel_locks_[i]);
                        rgb = pixel.color_rgb(background_);
                        rgba = pixel.color_rgba(background_);
                }

                ASSERT(rgba[3] < 1 || !is_finite(rgba) || !is_finite(rgb)
                       || (rgb[0] == rgba[0] && rgb[1] == rgba[1] && rgb[2] == rgba[2]));
                ASSERT(rgba[3] > 0 || !is_finite(rgb) || (rgb == background_.color_rgb32()));

                std::memcpy(ptr_rgb, &rgb, RGB_PIXEL_SIZE);
                ptr_rgb += RGB_PIXEL_SIZE;

                std::memcpy(ptr_rgba, &rgba, RGBA_PIXEL_SIZE);
                ptr_rgba += RGBA_PIXEL_SIZE;
        }
        ASSERT(ptr_rgb == image_rgb->pixels.data() + image_rgb->pixels.size());
        ASSERT(ptr_rgba == image_rgba->pixels.data() + image_rgba->pixels.size());
}

#define TEMPLATE_N_T_C(N, T, C) template class Pixels<(N)-1, T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE_N_T_C)
}
