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

#include <GL/glcorearb.h>

namespace opengl
{
class ShaderHandle final
{
        GLuint m_shader = 0;

        void destroy() noexcept;
        void move(ShaderHandle* from) noexcept;

public:
        ShaderHandle(GLenum type);
        ~ShaderHandle();

        ShaderHandle(const ShaderHandle&) = delete;
        ShaderHandle& operator=(const ShaderHandle&) = delete;

        ShaderHandle(ShaderHandle&& from) noexcept;
        ShaderHandle& operator=(ShaderHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class ProgramHandle final
{
        GLuint m_program = 0;

        void destroy() noexcept;
        void move(ProgramHandle* from) noexcept;

public:
        ProgramHandle();
        ~ProgramHandle();

        ProgramHandle(const ProgramHandle&) = delete;
        ProgramHandle& operator=(const ProgramHandle&) = delete;

        ProgramHandle(ProgramHandle&& from) noexcept;
        ProgramHandle& operator=(ProgramHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class Texture2DHandle final
{
        GLuint m_texture = 0;

        void destroy() noexcept;
        void move(Texture2DHandle* from) noexcept;

public:
        Texture2DHandle();
        ~Texture2DHandle();

        Texture2DHandle(const Texture2DHandle&) = delete;
        Texture2DHandle& operator=(const Texture2DHandle&) = delete;

        Texture2DHandle(Texture2DHandle&& from) noexcept;
        Texture2DHandle& operator=(Texture2DHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class Texture2DMultisampleHandle final
{
        GLuint m_texture = 0;

        void destroy() noexcept;
        void move(Texture2DMultisampleHandle* from) noexcept;

public:
        Texture2DMultisampleHandle();
        ~Texture2DMultisampleHandle();

        Texture2DMultisampleHandle(const Texture2DMultisampleHandle&) = delete;
        Texture2DMultisampleHandle& operator=(const Texture2DMultisampleHandle&) = delete;

        Texture2DMultisampleHandle(Texture2DMultisampleHandle&& from) noexcept;
        Texture2DMultisampleHandle& operator=(Texture2DMultisampleHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class FramebufferHandle final
{
        GLuint m_framebuffer = 0;

        void destroy() noexcept;
        void move(FramebufferHandle* from) noexcept;

public:
        FramebufferHandle();
        ~FramebufferHandle();

        FramebufferHandle(const FramebufferHandle&) = delete;
        FramebufferHandle& operator=(const FramebufferHandle&) = delete;

        FramebufferHandle(FramebufferHandle&& from) noexcept;
        FramebufferHandle& operator=(FramebufferHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class BufferHandle final
{
        GLuint m_buffer = 0;

        void destroy() noexcept;
        void move(BufferHandle* from) noexcept;

public:
        BufferHandle(GLenum target);
        ~BufferHandle();

        BufferHandle(const BufferHandle&) = delete;
        BufferHandle& operator=(const BufferHandle&) = delete;

        BufferHandle(BufferHandle&& from) noexcept;
        BufferHandle& operator=(BufferHandle&& from) noexcept;

        operator GLuint() const noexcept;
};

class VertexArrayHandle final
{
        GLuint m_vertex_array = 0;

        void destroy() noexcept;
        void move(VertexArrayHandle* from) noexcept;

public:
        VertexArrayHandle();
        ~VertexArrayHandle();

        VertexArrayHandle(const VertexArrayHandle&) = delete;
        VertexArrayHandle& operator=(const VertexArrayHandle&) = delete;

        VertexArrayHandle(VertexArrayHandle&& from) noexcept;
        VertexArrayHandle& operator=(VertexArrayHandle&& from) noexcept;

        operator GLuint() const noexcept;
};
}
