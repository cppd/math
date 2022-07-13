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

#include "pixel_samples.h"

#include <optional>
#include <tuple>

namespace ns::painter
{
template <typename Color>
class Pixel final
{
        using T = typename Color::DataType;

        ColorSamples<Color> color_;
        BackgroundSamples<Color> background_;

public:
        Pixel()
        {
        }

        void merge(const ColorSamples<Color>& samples)
        {
                color_.merge(samples);
        }

        void merge(const BackgroundSamples<Color>& samples)
        {
                background_.merge(samples);
        }

        [[nodiscard]] std::optional<Color> color(const Color& background_color, const T background_contribution) const
        {
                if (color_.empty())
                {
                        return std::nullopt;
                }

                if (background_.empty())
                {
                        return color_.sum() / color_.sum_weight();
                }

                const PixelSamples<Color> p = merge_color_and_background(color_, background_, background_contribution);

                const T sum = p.color_weight + p.background_weight;

                if (p.color_weight == sum || (p.color_weight / sum) == 1)
                {
                        return p.color / sum;
                }

                return (p.color + p.background_weight * background_color) / sum;
        }

        [[nodiscard]] std::optional<std::tuple<Color, T>> color_alpha(const T background_contribution) const
        {
                if (color_.empty())
                {
                        return std::nullopt;
                }

                if (background_.empty())
                {
                        return std::tuple<Color, T>(color_.sum() / color_.sum_weight(), 1);
                }

                const PixelSamples<Color> p = merge_color_and_background(color_, background_, background_contribution);

                const T sum = p.color_weight + p.background_weight;

                return std::make_tuple(p.color / sum, p.color_weight / sum);
        }
};
}
