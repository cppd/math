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

#include "compute_memory.h"

#include "graphics/opengl/functions.h"

namespace gpu_opengl
{
DftMemoryFftGlobalData::DftMemoryFftGlobalData(float two_pi_div_m, int m_div_2) : m_data(sizeof(Data), GL_MAP_WRITE_BIT)
{
        Data d;
        d.m_div_2 = m_div_2;
        d.two_pi_div_m = two_pi_div_m;
        opengl::map_and_write_to_buffer(m_data, d);
}

void DftMemoryFftGlobalData::bind() const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, DATA_BINDING, m_data);
}

//

void DftMemoryFftGlobalBuffer::set(const opengl::Buffer& buffer)
{
        m_buffer = buffer;
}

void DftMemoryFftGlobalBuffer::bind() const
{
        ASSERT(m_buffer > 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING, m_buffer);
}
}

#endif
