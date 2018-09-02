/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/color/colors.h"
#include "com/mat.h"

#include <memory>
#include <string>
#include <vector>

class Text final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        Text(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix);
        ~Text();

        void set_color(const Color& color) const;

        void draw(const std::vector<std::string>& text);
};
