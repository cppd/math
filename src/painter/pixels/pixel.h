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

#include "samples/background.h"
#include "samples/background_merge.h"
#include "samples/color.h"
#include "samples/color_merge.h"
#include "samples/merge.h"

#include <src/com/log.h>
#include <src/numerical/vector.h>

namespace ns::painter::pixels
{
template <typename Color>
class Pixel final
{
        samples::ColorSamples<Color> color_samples_;
        samples::BackgroundSamples<Color> background_samples_;

public:
        void merge(const samples::ColorSamples<Color>& samples)
        {
                color_samples_ = samples::merge_color_samples(color_samples_, samples);
        }

        void merge(const samples::BackgroundSamples<Color>& samples)
        {
                background_samples_ = samples::merge_background_samples(background_samples_, samples);
        }

        [[nodiscard]] Vector<3, float> color_rgb(const Background<Color>& background) const
        {
                const auto color = samples::merge_color(color_samples_, background_samples_, background);
                if (!color)
                {
                        return background.color_rgb32();
                }

                const Vector<3, float> rgb = color->rgb32();
                if (!is_finite(rgb))
                {
                        LOG("Not finite RGB color " + to_string(rgb));
                }
                return rgb;
        }

        [[nodiscard]] Vector<4, float> color_rgba(const Background<Color>& background) const
        {
                const auto color_alpha = samples::merge_color_alpha(color_samples_, background_samples_, background);
                if (!color_alpha)
                {
                        return Vector<4, float>(0);
                }

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
};
}
