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

#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::pixels::samples
{
template <std::size_t COUNT, typename T, typename Color>
[[nodiscard]] std::optional<BackgroundSamples<COUNT, Color>> create_background_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights);

template <std::size_t COUNT, typename T, typename Color>
[[nodiscard]] std::optional<ColorSamples<COUNT, Color>> create_color_samples(
        const std::vector<std::optional<Color>>& colors,
        const std::vector<T>& weights);
}
