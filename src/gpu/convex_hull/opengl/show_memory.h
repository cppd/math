/*
Copyright (C) 2017-2020 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include "com/matrix.h"
#include "graphics/opengl/buffers.h"

namespace gpu_opengl
{
class ConvexHullShaderMemory final
{
        static constexpr int DATA_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;

        opengl::Buffer m_data_buffer;
        const opengl::Buffer* m_points = nullptr;

        struct Data
        {
                mat4f matrix;
                float brightness;
        };

public:
        ConvexHullShaderMemory();

        void set_matrix(const mat4& matrix) const;
        void set_brightness(float brightness) const;
        void set_points(const opengl::Buffer& points);

        void bind() const;
};
}

#endif
