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
#include "samples_merge.h"

#include <optional>
#include <tuple>

namespace ns::painter::pixels
{
template <typename Color>
class Pixel final
{
        ColorSamples<Color> color_;
        BackgroundSamples<Color> background_;

public:
        Pixel()
        {
        }

        void merge(const ColorSamples<Color>& samples)
        {
                merge_color_samples(&color_, samples);
        }

        void merge(const BackgroundSamples<Color>& samples)
        {
                merge_background_samples(&background_, samples);
        }

        [[nodiscard]] std::optional<Color> color(
                const Color& background_color,
                const typename Color::DataType background_contribution) const
        {
                return merge_color(color_, background_, background_color, background_contribution);
        }

        [[nodiscard]] std::optional<std::tuple<Color, typename Color::DataType>> color_alpha(
                const typename Color::DataType background_contribution) const
        {
                return merge_color_alpha(color_, background_, background_contribution);
        }
};
}
