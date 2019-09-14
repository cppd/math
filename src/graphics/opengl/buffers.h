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
class BufferMapper
{
        GLuint m_buffer;
        GLbitfield m_access;
        unsigned long long m_length;
        void* m_pointer;

public:
        template <typename Buffer>
        BufferMapper(const Buffer& buffer, unsigned long long offset, unsigned long long length, GLbitfield access)
                : m_buffer(buffer), m_access(access), m_length(length)
        {
                ASSERT(((access & GL_MAP_WRITE_BIT) && (buffer.flags() & GL_MAP_WRITE_BIT)) ||
                       ((access & GL_MAP_READ_BIT) && (buffer.flags() & GL_MAP_READ_BIT)));
                ASSERT(length > 0 && offset + length <= buffer.size());

                m_pointer = glMapNamedBufferRange(m_buffer, offset, length, access);

                if (!m_pointer)
                {
                        error("Failed to map buffer");
                }
        }

        template <typename Buffer>
        BufferMapper(const Buffer& buffer, GLbitfield access) : m_buffer(buffer), m_access(access), m_length(buffer.size())
        {
                ASSERT(((access & GL_MAP_WRITE_BIT) && (buffer.flags() & GL_MAP_WRITE_BIT)) ||
                       ((access & GL_MAP_READ_BIT) && (buffer.flags() & GL_MAP_READ_BIT)));

                m_pointer = glMapNamedBufferRange(m_buffer, 0, buffer.size(), access);

                if (!m_pointer)
                {
                        error("Failed to map buffer");
                }
        }

        ~BufferMapper()
        {
                glUnmapNamedBuffer(m_buffer);
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                size_t size = storage_size(data);
                ASSERT(size <= m_length);
                std::memcpy(m_pointer, data.data(), size);
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> write(const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                size_t size = sizeof(data);
                ASSERT(size <= m_length);
                std::memcpy(m_pointer, &data, size);
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(GLintptr offset, const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                size_t size = storage_size(data);
                ASSERT(offset >= 0 && offset + size <= m_length);
                std::memcpy(static_cast<char*>(m_pointer) + offset, data.data(), size);
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> write(GLintptr offset, const T& data) const
        {
                ASSERT(m_access & GL_MAP_WRITE_BIT);
                size_t size = sizeof(data);
                ASSERT(offset >= 0 && offset + size <= m_length);
                std::memcpy(static_cast<char*>(m_pointer) + offset, &data, size);
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> read(T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                size_t size = storage_size(*data);
                ASSERT(size <= m_length);
                std::memcpy(data->data(), m_pointer, size);
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                size_t size = sizeof(data);
                ASSERT(size <= m_length);
                std::memcpy(&data, m_pointer, size);
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> read(GLintptr offset, T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                size_t size = storage_size(*data);
                ASSERT(offset >= 0 && offset + size <= m_length);
                std::memcpy(data->data(), static_cast<const char*>(m_pointer) + offset, size);
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(GLintptr offset, T* data) const
        {
                ASSERT(m_access & GL_MAP_READ_BIT);
                size_t size = sizeof(data);
                ASSERT(offset >= 0 && offset + size <= m_length);
                std::memcpy(&data, static_cast<const char*>(m_pointer) + offset, size);
        }
};

class Buffer final
{
        BufferHandle m_buffer;
        unsigned long long m_size;
        GLbitfield m_flags;

public:
        Buffer(unsigned long long size, GLbitfield flags) : m_size(size), m_flags(flags)
        {
                glNamedBufferStorage(m_buffer, m_size, nullptr, m_flags);
        }

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(std::declval<T>().data())>>
        explicit Buffer(const T& data, GLbitfield flags) : m_size(storage_size(data)), m_flags(flags)
        {
                glNamedBufferStorage(m_buffer, m_size, data.data(), m_flags);
        }

        unsigned long long size() const
        {
                return m_size;
        }

        GLbitfield flags() const
        {
                return m_flags;
        }

        operator GLuint() const
        {
                return m_buffer;
        }
};

class UniformBuffer final
{
        BufferHandle m_buffer;
        GLsizeiptr m_data_size;

        void copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const;

public:
        explicit UniformBuffer(GLsizeiptr data_size);

        void bind(GLuint point) const;

        GLsizeiptr size() const;

        template <typename T>
        void copy(GLintptr offset, const T& data) const
        {
                copy_to(offset, &data, sizeof(data));
        }

        template <typename T>
        void copy(const T& data) const
        {
                ASSERT(size() == sizeof(data));

                copy_to(0, &data, sizeof(data));
        }
};

class StorageBuffer final
{
        BufferHandle m_buffer;
        GLsizeiptr m_data_size;

        void copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const;
        void copy_from(GLintptr offset, void* data, GLsizeiptr data_size) const;

public:
        explicit StorageBuffer(GLsizeiptr data_size);

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(std::declval<T>().data())>>
        explicit StorageBuffer(const T& data) : StorageBuffer(storage_size(data))
        {
                write(data);
        }

        void bind(GLuint point) const;

        GLsizeiptr size() const;

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> write(const T& data) const
        {
                copy_to(0, data.data(), storage_size(data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> write(const T& data) const
        {
                ASSERT(size() == sizeof(data));
                copy_to(0, &data, sizeof(data));
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> read(T* data) const
        {
                copy_from(0, data->data(), storage_size(*data));
        }

        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(T* data) const
        {
                copy_from(0, data, sizeof(T));
        }
};

class VertexArray final
{
        VertexArrayHandle m_vertex_array;

public:
        void bind() const;

        void attrib(GLuint attrib_index, GLint size, GLenum type, const Buffer& buffer, GLintptr offset, GLsizei stride) const;
        void attrib_i(GLuint attrib_index, GLint size, GLenum type, const Buffer& buffer, GLintptr offset, GLsizei stride) const;
};

class Texture final
{
        // template <typename T>
        // static constexpr bool is_float_buffer = (is_vector<T> || is_array<T>)&&std::is_same_v<typename T::value_type, GLfloat>;

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

        int width() const;
        int height() const;
        GLenum format() const;

        operator GLuint() const;
};

class DepthFramebuffer final
{
        Texture m_depth;
        FramebufferHandle m_framebuffer;

public:
        DepthFramebuffer(GLenum depth_format, GLsizei width, GLsizei height);

        operator GLuint() const;
        const Texture& texture() const;
};

class ColorFramebuffer final
{
        Texture m_color;
        FramebufferHandle m_framebuffer;

public:
        ColorFramebuffer(GLenum color_format, GLsizei width, GLsizei height);

        operator GLuint() const;
        const Texture& texture() const;
};

class ColorDepthFramebufferMultisample final
{
        RenderbufferHandle m_color;
        RenderbufferHandle m_depth;
        FramebufferHandle m_framebuffer;

public:
        ColorDepthFramebufferMultisample(GLenum color_format, GLenum depth_format, GLsizei samples, GLsizei width,
                                         GLsizei height);

        operator GLuint() const;
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
