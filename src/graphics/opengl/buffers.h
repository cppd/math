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

#include "com/container.h"
#include "com/error.h"
#include "com/span.h"
#include "com/type/detect.h"
#include "graphics/opengl/functions.h"
#include "graphics/opengl/objects.h"

#include <cstring>
#include <type_traits>

namespace opengl
{
class Buffer final
{
        BufferHandle m_buffer;
        unsigned long long m_size;
        GLbitfield m_flags;

        Buffer(unsigned long long size, const void* data, GLbitfield flags);

public:
        Buffer(unsigned long long size, GLbitfield flags) : Buffer(size, nullptr, flags)
        {
        }

        template <typename T>
        Buffer(unsigned long long size, GLbitfield flags, const T& data) : Buffer(size, data_pointer(data), flags)
        {
                if (size != data_size(data))
                {
                        error("Buffer size and data size are not equal");
                }
        }

        unsigned long long size() const;
        GLbitfield flags() const;
        operator GLuint() const&;
        operator GLuint() const&& = delete;
};

class BufferMapper final
{
        GLuint m_buffer;
        GLbitfield m_access;
        unsigned long long m_length;
        void* m_pointer;

public:
        BufferMapper(const Buffer& buffer, unsigned long long offset, unsigned long long length, GLbitfield access);
        BufferMapper(const Buffer& buffer, GLbitfield access);
        ~BufferMapper();

        BufferMapper(const BufferMapper&) = delete;
        BufferMapper(BufferMapper&&) = delete;
        BufferMapper& operator=(const BufferMapper&) = delete;
        BufferMapper& operator=(BufferMapper&&) = delete;

        template <typename T>
        void write(const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                ASSERT(data_size(data) <= m_length);
                std::memcpy(m_pointer, data_pointer(data), data_size(data));
        }

        template <typename T>
        void write(unsigned long long offset, const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                ASSERT(offset + data_size(data) <= m_length);
                std::memcpy(static_cast<char*>(m_pointer) + offset, data_pointer(data), data_size(data));
        }

        template <typename T>
        void read(T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                ASSERT(data_size(*data) <= m_length);
                std::memcpy(data_pointer(*data), m_pointer, data_size(*data));
        }

        template <typename T>
        void read(unsigned long long offset, T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                ASSERT(offset + data_size(data) <= m_length);
                std::memcpy(data_pointer(*data), static_cast<const char*>(m_pointer) + offset, data_size(data));
        }
};

template <typename T>
void map_and_write_to_buffer(const Buffer& buffer, unsigned long long offset, const T& data)
{
        BufferMapper map(buffer, offset, data_size(data), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
        map.write(data);
}

template <typename T>
void map_and_write_to_buffer(const Buffer& buffer, const T& data)
{
        BufferMapper map(buffer, 0, data_size(data), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        map.write(data);
}

template <typename T>
void map_and_read_from_buffer(const Buffer& buffer, unsigned long long offset, T* data)
{
        BufferMapper map(buffer, offset, data_size(*data), GL_MAP_READ_BIT);
        map.read(data);
}

template <typename T>
void map_and_read_from_buffer(const Buffer& buffer, T* data)
{
        BufferMapper map(buffer, 0, data_size(*data), GL_MAP_READ_BIT);
        map.read(data);
}

class VertexArray final
{
        VertexArrayHandle m_vertex_array;

public:
        void bind() const;

        void attrib(GLuint attrib_index, GLint size, GLenum type, const Buffer& buffer, GLintptr offset, GLsizei stride)
                const;
        void attrib_i(
                GLuint attrib_index,
                GLint size,
                GLenum type,
                const Buffer& buffer,
                GLintptr offset,
                GLsizei stride) const;
};

class Texture final
{
        // template <typename T>
        // static constexpr bool is_float_buffer = (is_vector<T> || is_array<T>)&&std::is_same_v<typename T::value_type, GLfloat>;

        struct ClearColorValue
        {
                GLenum format;
                GLenum type;
                union
                {
                        GLfloat gl_float[4];
                        GLint gl_int[4];
                        GLuint gl_uint[4];
                } data;
        };

        static ClearColorValue clear_color_value(GLenum format);

        Texture2DHandle m_texture;
        GLenum m_format;
        int m_width;
        int m_height;

public:
        Texture(GLenum format, GLsizei width, GLsizei height, const Span<const std::uint_least8_t>& srgb_pixels);

        Texture(GLenum format, GLsizei width, GLsizei height);

        GLuint64 image_handle_write_only() const;
        GLuint64 image_handle_read_only() const;
        GLuint64 image_handle_read_write() const;

        GLuint64 texture_handle() const;

        // void bind_image_read_only(GLuint unit) const;
        // void bind_image_write_only(GLuint unit) const;
        // void bind_image_read_write(GLuint unit) const;

        void clear() const;
        void clear(int offset_x, int offset_y, int width, int height) const;

        int width() const;
        int height() const;
        GLenum format() const;

        operator GLuint() const&;
        operator GLuint() const&& = delete;
};

class DepthFramebuffer final
{
        Texture m_depth;
        FramebufferHandle m_framebuffer;

public:
        DepthFramebuffer(GLenum depth_format, GLsizei width, GLsizei height);

        operator GLuint() const&;
        operator GLuint() const&& = delete;
        const Texture& texture() const;
};

class ColorFramebuffer final
{
        Texture m_color;
        FramebufferHandle m_framebuffer;

public:
        ColorFramebuffer(GLenum color_format, GLsizei width, GLsizei height);

        operator GLuint() const&;
        operator GLuint() const&& = delete;
        const Texture& texture() const;
};

class ColorDepthFramebufferMultisample final
{
        RenderbufferHandle m_color;
        RenderbufferHandle m_depth;
        FramebufferHandle m_framebuffer;

public:
        ColorDepthFramebufferMultisample(
                GLenum color_format,
                GLenum depth_format,
                GLsizei samples,
                GLsizei width,
                GLsizei height);

        operator GLuint() const&;
        operator GLuint() const&& = delete;
};

class FramebufferBinder final
{
        GLuint m_draw_framebuffer;
        GLuint m_read_framebuffer;

public:
        FramebufferBinder(GLuint framebuffer);
        ~FramebufferBinder();

        FramebufferBinder(const FramebufferBinder&) = delete;
        FramebufferBinder(FramebufferBinder&&) = delete;
        FramebufferBinder& operator=(const FramebufferBinder&) = delete;
        FramebufferBinder& operator=(FramebufferBinder&&) = delete;
};
}

#endif
