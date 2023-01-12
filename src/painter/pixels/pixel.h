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

#include "color_contribution.h"
#include "samples_background.h"
#include "samples_color.h"
#include "samples_merge.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::painter::pixels
{
template <typename Color>
class Background final
{
        Color color_;
        Vector<3, float> color_rgb32_ = color_.rgb32();
        typename Color::DataType contribution_ = sample_color_contribution(color_);

public:
        explicit Background(const Color& color)
                : color_(color)
        {
                if (!color_.is_finite())
                {
                        error("Not finite background " + to_string(color_));
                }

                if (!is_finite(color_rgb32_))
                {
                        error("Not finite background RGB " + to_string(color_rgb32_));
                }

                if (!std::isfinite(contribution_))
                {
                        error("Not finite background contribution " + to_string(contribution_));
                }
        }

        [[nodiscard]] const Color& color() const
        {
                return color_;
        }

        [[nodiscard]] const Vector<3, float>& color_rgb32() const
        {
                return color_rgb32_;
        }

        [[nodiscard]] typename Color::DataType contribution() const
        {
                return contribution_;
        }
};

template <typename Color>
class Pixel final
{
        ColorSamples<Color> color_;
        BackgroundSamples<Color> background_;

public:
        void merge(const ColorSamples<Color>& samples)
        {
                merge_color_samples(&color_, samples);
        }

        void merge(const BackgroundSamples<Color>& samples)
        {
                merge_background_samples(&background_, samples);
        }

        [[nodiscard]] Vector<3, float> color_rgb(const Background<Color>& background) const
        {
                const auto color = merge_color(color_, background_, background.color(), background.contribution());
                if (color)
                {
                        const Vector<3, float> rgb = color->rgb32();
                        if (!is_finite(rgb))
                        {
                                LOG("Not finite RGB color " + to_string(rgb));
                        }
                        return rgb;
                }
                return background.color_rgb32();
        }

        [[nodiscard]] Vector<4, float> color_rgba(const Background<Color>& background) const
        {
                const auto color_alpha = merge_color_alpha(color_, background_, background.contribution());
                if (color_alpha)
                {
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
                return Vector<4, float>(0);
        }
};
}
