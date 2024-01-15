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

#pragma once

#include "background.h"
#include "color.h"

#include <src/painter/pixels/background.h>

#include <cstddef>
#include <optional>
#include <tuple>

namespace ns::painter::pixels::samples
{
template <std::size_t COUNT, typename Color>
[[nodiscard]] std::optional<Color> merge_color(
        const ColorSamples<COUNT, Color>& color_samples,
        const BackgroundSamples<COUNT, Color>& background_samples,
        const Background<Color>& background);

template <std::size_t COUNT, typename Color>
[[nodiscard]] std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<COUNT, Color>& color_samples,
        const BackgroundSamples<COUNT, Color>& background_samples,
        const Background<Color>& background);
}
