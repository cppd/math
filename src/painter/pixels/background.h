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

#include "color_contribution.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::painter::pixels
{
template <typename Color>
class Background final
{
        Color color_;
        numerical::Vector<3, float> color_rgb32_ = color_.rgb32();
        Color::DataType contribution_ = sample_color_contribution(color_);

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

        [[nodiscard]] const numerical::Vector<3, float>& color_rgb32() const
        {
                return color_rgb32_;
        }

        [[nodiscard]] Color::DataType contribution() const
        {
                return contribution_;
        }
};
}
