/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "background.h"
#include "pixel.h"
#include "pixel_filter.h"
#include "pixel_region.h"

#include <src/com/global_index.h>
#include <src/com/spinlock.h>
#include <src/image/image.h>
#include <src/numerical/vector.h>
#include <src/painter/painter.h>

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <vector>

namespace ns::painter::pixels
{
template <std::size_t N, typename T, typename Color>
class Pixels final
{
        static constexpr std::size_t FILTER_SAMPLE_COUNT = 4;

        const PixelFilter<N, T> filter_;
        const std::array<int, N> screen_size_;
        const GlobalIndex<N, long long> global_index_{screen_size_};
        const PixelRegion<N> pixel_region_{screen_size_, filter_.integer_radius()};
        const Background<Color> background_;
        Notifier<N>* const notifier_;

        std::vector<Pixel<FILTER_SAMPLE_COUNT, Color>> pixels_{static_cast<std::size_t>(global_index_.count())};
        mutable std::vector<Spinlock> pixel_locks_{pixels_.size()};

        void add_samples(
                const std::array<int, N>& region_pixel,
                const std::array<int, N>& sample_pixel,
                const std::vector<numerical::Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors);

public:
        Pixels(const std::array<int, N>& screen_size,
               const std::type_identity_t<Color>& background,
               Notifier<N>* notifier);

        void add_samples(
                const std::array<int, N>& pixel,
                const std::vector<numerical::Vector<N, T>>& points,
                const std::vector<std::optional<Color>>& colors);

        void images(image::Image<N>* image_rgb, image::Image<N>* image_rgba) const;
};
}
