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

#include <src/color/color.h>

#include <algorithm>

namespace ns::painter::shapes::mesh
{
template <typename T, typename Color>
class Material final
{
        T metalness_;
        T roughness_;
        Color color_;
        T alpha_;
        int image_;

public:
        Material(const T metalness, const T roughness, const color::Color& color, const int image, const T alpha)
                : metalness_(std::clamp<T>(metalness, 0, 1)),
                  roughness_(std::clamp<T>(roughness, 0, 1)),
                  color_(to_color<Color>(color).clamp(0, 1)),
                  alpha_(std::clamp<T>(alpha, 0, 1)),
                  image_(image)
        {
        }

        [[nodiscard]] T metalness() const
        {
                return metalness_;
        }

        [[nodiscard]] T roughness() const
        {
                return roughness_;
        }

        [[nodiscard]] const Color& color() const
        {
                return color_;
        }

        [[nodiscard]] T alpha() const
        {
                return alpha_;
        }

        [[nodiscard]] int image() const
        {
                return image_;
        }
};
}
