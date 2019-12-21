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

#if defined(OPENGL_FOUND)

#include "show_memory.h"

#include "com/error.h"

namespace gpu_opengl
{
ConvexHullShaderMemory::ConvexHullShaderMemory() : m_data_buffer(sizeof(Data), GL_MAP_WRITE_BIT)
{
}

void ConvexHullShaderMemory::set_matrix(const mat4& matrix) const
{
        decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
        opengl::map_and_write_to_buffer(m_data_buffer, offsetof(Data, matrix), m);
}

void ConvexHullShaderMemory::set_brightness(float brightness) const
{
        decltype(Data().brightness) b = brightness;
        opengl::map_and_write_to_buffer(m_data_buffer, offsetof(Data, brightness), b);
}

void ConvexHullShaderMemory::set_points(const opengl::Buffer& points)
{
        m_points = &points;
}

void ConvexHullShaderMemory::bind() const
{
        ASSERT(m_points);

        glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BINDING, m_data_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_BINDING, *m_points);
}
}

#endif
