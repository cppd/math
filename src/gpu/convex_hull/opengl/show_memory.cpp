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

#include "show_memory.h"

#include "com/error.h"

namespace gpu_opengl
{
ConvexHullShaderMemory::ConvexHullShaderMemory() : m_buffer(sizeof(Data))
{
}

void ConvexHullShaderMemory::set_matrix(const mat4& matrix) const
{
        decltype(Data().matrix) m = transpose(to_matrix<float>(matrix));
        m_buffer.copy(offsetof(Data, matrix), m);
}

void ConvexHullShaderMemory::set_brightness(float brightness) const
{
        decltype(Data().brightness) b = brightness;
        m_buffer.copy(offsetof(Data, brightness), b);
}

void ConvexHullShaderMemory::set_points(const opengl::StorageBuffer& points)
{
        m_points = &points;
}

void ConvexHullShaderMemory::bind() const
{
        ASSERT(m_points);

        m_buffer.bind(DATA_BINDING);
        m_points->bind(POINTS_BINDING);
}
}
