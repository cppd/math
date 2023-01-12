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

#include "samples_background.h"
#include "samples_color.h"

namespace ns::painter::pixels
{
template <typename Color>
void merge_color_samples(ColorSamples<Color>* dst, const ColorSamples<Color>& samples);

template <typename Color>
void merge_background_samples(BackgroundSamples<Color>* dst, const BackgroundSamples<Color>& samples);

template <typename Color, typename ColorDataType>
[[nodiscard]] std::optional<Color> merge_color(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const Color& background_color,
        ColorDataType background_contribution);

template <typename Color, typename ColorDataType>
[[nodiscard]] std::optional<std::tuple<Color, typename Color::DataType>> merge_color_alpha(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        ColorDataType background_contribution);
}
