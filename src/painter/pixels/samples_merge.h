/*
Copyright (C) 2017-2023 Topological Manifold

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
#include "samples_background.h"
#include "samples_color.h"

#include <optional>

namespace ns::painter::pixels
{
template <typename Color>
[[nodiscard]] ColorSamples<Color> merge_color_samples(const ColorSamples<Color>& a, const ColorSamples<Color>& b);

template <typename Color>
[[nodiscard]] BackgroundSamples<Color> merge_background_samples(
        const BackgroundSamples<Color>& a,
        const BackgroundSamples<Color>& b);

template <typename Color>
[[nodiscard]] std::optional<Color> merge_color(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background);

template <typename Color>
[[nodiscard]] std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color_samples,
        const BackgroundSamples<Color>& background_samples,
        const Background<Color>& background);
}
