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

#include "background.h"
#include "color.h"

#include <src/com/error.h>

namespace ns::painter::sample
{
template <typename Color>
struct PixelSamples final
{
        Color color;
        typename Color::DataType color_weight;
        typename Color::DataType background_weight;
};

template <typename Color>
[[nodiscard]] PixelSamples<Color> merge_color_and_background(
        const ColorSamples<Color>& color,
        const BackgroundSamples<Color>& background,
        const typename Color::DataType background_contribution)
{
        using T = typename Color::DataType;

        ASSERT(!color.empty());
        ASSERT(!background.empty());

        PixelSamples<Color> res{
                .color = color.sum(),
                .color_weight = color.sum_weight(),
                .background_weight = background.sum_weight()};

        const T background_min_contribution = background.min_weight() * background_contribution;
        const T background_max_contribution = background.max_weight() * background_contribution;

        if (background_min_contribution < color.min_contribution())
        {
                res.color += color.min();
                res.color_weight += color.min_weight();
        }
        else
        {
                res.background_weight += background.min_weight();
        }

        if (background_max_contribution > color.max_contribution())
        {
                res.color += color.max();
                res.color_weight += color.max_weight();
        }
        else
        {
                res.background_weight += background.max_weight();
        }

        return res;
}
}
