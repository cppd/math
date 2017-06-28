/*
Copyright (C) 2017 Topological Manifold

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

#include "gl/gl_objects.h"
#include <glm/vec3.hpp>

class ColorSpaceConverter
{
        ComputeProgram m_prog;
        bool m_to_rgb;

public:
        ColorSpaceConverter(bool to_rgb);
        void convert(const Texture2D& tex) const;
};

class ColorSpaceConverterToRGB : public ColorSpaceConverter
{
public:
        ColorSpaceConverterToRGB() : ColorSpaceConverter(true)
        {
        }
};

// Вместо luminosity из GLM, так как там используются
// коэффициенты 0.33, 0.59 и 0.11, что в сумме больше 1.
constexpr float luminosity(glm::vec3 a)
{
        return 0.299f * a.r + 0.587f * a.g + 0.114f * a.b;
}
