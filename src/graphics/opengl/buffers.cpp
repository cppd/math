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
void Texture2DHandle::destroy() noexcept
{
        if (m_texture != 0)
        {
                glDeleteTextures(1, &m_texture);
        }
}

void Texture2DHandle::move(Texture2DHandle* from) noexcept
{
        m_texture = from->m_texture;
        from->m_texture = 0;
}

Texture2DHandle::Texture2DHandle()
{
        glCreateTextures(GL_TEXTURE_2D, 1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2DHandle::~Texture2DHandle()
{
        destroy();
}

Texture2DHandle::Texture2DHandle(Texture2DHandle&& from) noexcept
{
        move(&from);
}

Texture2DHandle& Texture2DHandle::operator=(Texture2DHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Texture2DHandle::operator GLuint() const noexcept
{
        return m_texture;
}

//

void FramebufferHandle::destroy() noexcept
{
        if (m_framebuffer != 0)
        {
                glDeleteFramebuffers(1, &m_framebuffer);
        }
}

void FramebufferHandle::move(FramebufferHandle* from) noexcept
{
        m_framebuffer = from->m_framebuffer;
        from->m_framebuffer = 0;
}

FramebufferHandle::FramebufferHandle()
{
        glCreateFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FramebufferHandle::~FramebufferHandle()
{
        destroy();
}

FramebufferHandle::FramebufferHandle(FramebufferHandle&& from) noexcept
{
        move(&from);
}

FramebufferHandle& FramebufferHandle::operator=(FramebufferHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

FramebufferHandle::operator GLuint() const noexcept
{
        return m_framebuffer;
}

//

void BufferHandle::destroy() noexcept
{
        if (m_buffer != 0)
        {
                glDeleteBuffers(1, &m_buffer);
        }
}

void BufferHandle::move(BufferHandle* from) noexcept
{
        m_buffer = from->m_buffer;
        from->m_buffer = 0;
}

BufferHandle::BufferHandle(GLenum target)
{
        glCreateBuffers(1, &m_buffer);
        glBindBuffer(target, m_buffer);
        glBindBuffer(target, 0);
}

BufferHandle::~BufferHandle()
{
        destroy();
}

BufferHandle::BufferHandle(BufferHandle&& from) noexcept
{
        move(&from);
}

BufferHandle& BufferHandle::operator=(BufferHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

BufferHandle::operator GLuint() const noexcept
{
        return m_buffer;
}

//

void VertexArrayHandle::destroy() noexcept
{
        if (m_vertex_array != 0)
        {
                glDeleteVertexArrays(1, &m_vertex_array);
        }
}

void VertexArrayHandle::move(VertexArrayHandle* from) noexcept
{
        m_vertex_array = from->m_vertex_array;
        from->m_vertex_array = 0;
}

VertexArrayHandle::VertexArrayHandle()
{
        glCreateVertexArrays(1, &m_vertex_array);
}

VertexArrayHandle::~VertexArrayHandle()
{
        destroy();
}

VertexArrayHandle::VertexArrayHandle(VertexArrayHandle&& from) noexcept
{
        move(&from);
}

VertexArrayHandle& VertexArrayHandle::operator=(VertexArrayHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

VertexArrayHandle::operator GLuint() const noexcept
{
        return m_vertex_array;
}

//

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
