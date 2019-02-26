/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/matrix.h"
#include "graphics/opengl/buffers.h"

#include <memory>

namespace gpgpu_opengl
{
struct PencilSketchShow
{
        virtual ~PencilSketchShow() = default;

        virtual void draw() = 0;
};

std::unique_ptr<PencilSketchShow> create_pencil_sketch_show(const opengl::TextureRGBA32F& source, bool source_is_srgb,
                                                            const opengl::TextureImage& objects, const mat4& matrix);
}
