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

#include "objects.h"

#include "graphics/opengl/functions/opengl_functions.h"

namespace opengl
{
void ShaderHandle::destroy() noexcept
{
        if (m_shader != 0)
        {
                glDeleteShader(m_shader);
        }
}

void ShaderHandle::move(ShaderHandle* from) noexcept
{
        m_shader = from->m_shader;
        from->m_shader = 0;
}

ShaderHandle::ShaderHandle(GLenum type)
{
        m_shader = glCreateShader(type);
}

ShaderHandle::~ShaderHandle()
{
        destroy();
}

ShaderHandle::ShaderHandle(ShaderHandle&& from) noexcept
{
        move(&from);
}

ShaderHandle& ShaderHandle::operator=(ShaderHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

ShaderHandle::operator GLuint() const noexcept
{
        return m_shader;
}

//

void ProgramHandle::destroy() noexcept
{
        if (m_program != 0)
        {
                glDeleteProgram(m_program);
        }
}

void ProgramHandle::move(ProgramHandle* from) noexcept
{
        m_program = from->m_program;
        from->m_program = 0;
}

ProgramHandle::ProgramHandle()
{
        m_program = glCreateProgram();
}

ProgramHandle::~ProgramHandle()
{
        destroy();
}

ProgramHandle::ProgramHandle(ProgramHandle&& from) noexcept
{
        move(&from);
}

ProgramHandle& ProgramHandle::operator=(ProgramHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

ProgramHandle::operator GLuint() const noexcept
{
        return m_program;
}

//

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

void Texture2DMultisampleHandle::destroy() noexcept
{
        if (m_texture != 0)
        {
                glDeleteTextures(1, &m_texture);
        }
}

void Texture2DMultisampleHandle::move(Texture2DMultisampleHandle* from) noexcept
{
        m_texture = from->m_texture;
        from->m_texture = 0;
}

Texture2DMultisampleHandle::Texture2DMultisampleHandle()
{
        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_texture);
}

Texture2DMultisampleHandle::~Texture2DMultisampleHandle()
{
        destroy();
}

Texture2DMultisampleHandle::Texture2DMultisampleHandle(Texture2DMultisampleHandle&& from) noexcept
{
        move(&from);
}

Texture2DMultisampleHandle& Texture2DMultisampleHandle::operator=(Texture2DMultisampleHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Texture2DMultisampleHandle::operator GLuint() const noexcept
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
}
