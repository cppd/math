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

#include "buffers.h"

namespace
{
void copy_to_buffer(const opengl::BufferHandle& buffer, GLintptr offset, const void* data, GLsizeiptr data_size) noexcept
{
        void* map_memory_data = glMapNamedBufferRange(buffer, offset, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

        std::memcpy(map_memory_data, data, data_size);

        glUnmapNamedBuffer(buffer);
}

void copy_from_buffer(const opengl::BufferHandle& buffer, GLintptr offset, void* data, GLsizeiptr data_size) noexcept
{
        void* map_memory_data = glMapNamedBufferRange(buffer, offset, data_size, GL_MAP_READ_BIT);

        std::memcpy(data, map_memory_data, data_size);

        glUnmapNamedBuffer(buffer);
}
}

namespace opengl
{
void UniformBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const noexcept
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}

void StorageBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const noexcept
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}

void StorageBuffer::copy_from(GLintptr offset, void* data, GLsizeiptr data_size) const noexcept
{
        ASSERT(offset + data_size <= m_data_size);

        copy_from_buffer(m_buffer, offset, data, data_size);
}

void ArrayBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const noexcept
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}
}
