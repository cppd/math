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

#include <glm/mat4x4.hpp>
#include <memory>

class ConvexHull2D final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        ConvexHull2D(const TextureR32I& objects, const glm::mat4& mtx);
        ~ConvexHull2D();

        void reset_timer();
        void draw();
};
