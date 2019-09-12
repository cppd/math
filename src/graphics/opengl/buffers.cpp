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

#include "com/color/conversion_span.h"
#include "com/print.h"
#include "graphics/opengl/functions.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace
{
void copy_to_buffer(const opengl::BufferHandle& buffer, GLintptr offset, const void* data, GLsizeiptr data_size)
{
        void* map_memory_data = glMapNamedBufferRange(buffer, offset, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

        std::memcpy(map_memory_data, data, data_size);

        glUnmapNamedBuffer(buffer);
}

void copy_from_buffer(const opengl::BufferHandle& buffer, GLintptr offset, void* data, GLsizeiptr data_size)
{
        void* map_memory_data = glMapNamedBufferRange(buffer, offset, data_size, GL_MAP_READ_BIT);

        std::memcpy(data, map_memory_data, data_size);

        glUnmapNamedBuffer(buffer);
}

GLuint getIntegerValue(GLenum parameter)
{
        GLint v;
        glGetIntegerv(parameter, &v);
        return v;
}
}

namespace opengl
{
void UniformBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}

UniformBuffer::UniformBuffer(GLsizeiptr data_size) : m_data_size(data_size)
{
        glNamedBufferStorage(m_buffer, data_size, nullptr, GL_MAP_WRITE_BIT);
}

void UniformBuffer::bind(GLuint point) const
{
        glBindBufferBase(GL_UNIFORM_BUFFER, point, m_buffer);
}

GLsizeiptr UniformBuffer::size() const
{
        return m_data_size;
}

//

void StorageBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}

void StorageBuffer::copy_from(GLintptr offset, void* data, GLsizeiptr data_size) const
{
        ASSERT(offset + data_size <= m_data_size);

        copy_from_buffer(m_buffer, offset, data, data_size);
}

StorageBuffer::StorageBuffer(GLsizeiptr data_size) : m_data_size(data_size)
{
        glNamedBufferStorage(m_buffer, data_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
}

void StorageBuffer::bind(GLuint point) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, point, m_buffer);
}

GLsizeiptr StorageBuffer::size() const
{
        return m_data_size;
}

//

void ArrayBuffer::copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const
{
        ASSERT(offset + data_size <= m_data_size);

        copy_to_buffer(m_buffer, offset, data, data_size);
}

ArrayBuffer::ArrayBuffer(GLsizeiptr data_size) : m_data_size(data_size)
{
        glNamedBufferStorage(m_buffer, data_size, nullptr, GL_MAP_WRITE_BIT);
}

size_t ArrayBuffer::size() const
{
        return m_data_size;
}

ArrayBuffer::operator GLuint() const
{
        return m_buffer;
}

//

void VertexArray::bind() const
{
        glBindVertexArray(m_vertex_array);
}

void VertexArray::attrib(GLuint attrib_index, GLint size, GLenum type, const ArrayBuffer& array_buffer, GLintptr offset,
                         GLsizei stride) const
{
        GLuint binding_index = attrib_index;
        glVertexArrayAttribFormat(m_vertex_array, attrib_index, size, type, GL_FALSE, 0);
        glVertexArrayAttribBinding(m_vertex_array, attrib_index, binding_index);
        glVertexArrayVertexBuffer(m_vertex_array, binding_index, array_buffer, offset, stride);

        glEnableVertexArrayAttrib(m_vertex_array, attrib_index);
}

void VertexArray::attrib_i(GLuint attrib_index, GLint size, GLenum type, const ArrayBuffer& array_buffer, GLintptr offset,
                           GLsizei stride) const
{
        GLuint binding_index = attrib_index;
        glVertexArrayAttribIFormat(m_vertex_array, attrib_index, size, type, 0);
        glVertexArrayAttribBinding(m_vertex_array, attrib_index, binding_index);
        glVertexArrayVertexBuffer(m_vertex_array, binding_index, array_buffer, offset, stride);

        glEnableVertexArrayAttrib(m_vertex_array, attrib_index);
}

// void VertexArray::enable_attrib(GLuint index) const
//{
//        glEnableVertexArrayAttrib(m_vertex_array, index);
//}

//

Texture::Texture(GLenum format, GLsizei width, GLsizei height, const Span<const std::uint_least8_t>& srgb_pixels)
        : Texture(format, width, height)
{
        if (m_format == GL_SRGB8_ALPHA8)
        {
                static_assert(sizeof(std::uint_least8_t) == sizeof(GLubyte));

                ASSERT(4ull * width * height == srgb_pixels.size());

                glTextureSubImage2D(m_texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, srgb_pixels.data());

                return;
        }
        if (m_format == GL_RGBA32F)
        {
                ASSERT(4ull * width * height == srgb_pixels.size());

                const std::vector<float> buffer = color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);

                glTextureSubImage2D(m_texture, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, buffer.data());

                return;
        }
        if (m_format == GL_R32F)
        {
                ASSERT(1ull * width * height == srgb_pixels.size());

                std::vector<float> buffer = color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);

                glTextureSubImage2D(m_texture, 0, 0, 0, width, height, GL_RED, GL_FLOAT, buffer.data());

                return;
        }
        error("Unsupported pixels texture format " + std::to_string(static_cast<long long>(m_format)));
}

Texture::Texture(GLenum format, GLsizei width, GLsizei height) : m_format(format), m_width(width), m_height(height)
{
        ASSERT(width >= 0 && height >= 0);

        glTextureStorage2D(m_texture, 1, m_format, m_width, m_height);

        if (m_format == GL_SRGB8_ALPHA8 || m_format == GL_RGBA32F || m_format == GL_R32F)
        {
                glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                return;
        }
        if (m_format == GL_DEPTH_COMPONENT32 || m_format == GL_DEPTH_COMPONENT24 || m_format == GL_DEPTH_COMPONENT16)
        {
                glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(m_texture, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTextureParameteri(m_texture, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                return;
        }
        if (m_format == GL_R32I || m_format == GL_R32UI)
        {
                return;
        }
        error("Unsupported texture format " + std::to_string(static_cast<long long>(m_format)));
}

GLuint64 Texture::image_handle_write_only() const
{
        GLuint64 image_handle = glGetImageHandleARB(m_texture, 0, GL_FALSE, 0, m_format);
        if (!image_handle)
        {
                error("Failed to get image handle");
        }
        if (!glIsImageHandleResidentARB(image_handle))
        {
                glMakeImageHandleResidentARB(image_handle, GL_WRITE_ONLY);
        }
        return image_handle;
}

GLuint64 Texture::image_handle_read_only() const
{
        GLuint64 image_handle = glGetImageHandleARB(m_texture, 0, GL_FALSE, 0, m_format);
        if (!image_handle)
        {
                error("Failed to get image handle");
        }
        if (!glIsImageHandleResidentARB(image_handle))
        {
                glMakeImageHandleResidentARB(image_handle, GL_READ_ONLY);
        }
        return image_handle;
}

GLuint64 Texture::image_handle_read_write() const
{
        GLuint64 image_handle = glGetImageHandleARB(m_texture, 0, GL_FALSE, 0, m_format);
        if (!image_handle)
        {
                error("Failed to get image handle");
        }
        if (!glIsImageHandleResidentARB(image_handle))
        {
                glMakeImageHandleResidentARB(image_handle, GL_READ_WRITE);
        }
        return image_handle;
}

GLuint64 Texture::texture_handle() const
{
        GLuint64 texture_handle = glGetTextureHandleARB(m_texture);
        if (!texture_handle)
        {
                error("Failed to get texture handle");
        }
        if (!glIsTextureHandleResidentARB(texture_handle))
        {
                glMakeTextureHandleResidentARB(texture_handle);
        }
        return texture_handle;
}

// void Texture::bind_image_read_only(GLuint unit) const
//{
//        glBindImageTexture(unit, m_texture, 0, GL_FALSE, 0, GL_READ_ONLY, m_format);
//}

// void Texture::bind_image_write_only(GLuint unit) const
//{
//        glBindImageTexture(unit, m_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, m_format);
//}

// void Texture::bind_image_read_write(GLuint unit) const
//{
//        glBindImageTexture(unit, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, m_format);
//}

void Texture::clear() const
{
        if (m_format == GL_RGBA32F || m_format == GL_SRGB8_ALPHA8)
        {
                const std::array<GLfloat, 4> v = {0, 0, 0, 1};
                glClearTexImage(m_texture, 0, GL_RGBA, GL_FLOAT, v.data());
                return;
        }
        if (m_format == GL_R32I)
        {
                GLint v = 0;
                glClearTexImage(m_texture, 0, GL_RED_INTEGER, GL_INT, &v);
                return;
        }
        if (m_format == GL_R32UI)
        {
                GLuint v = 0;
                glClearTexImage(m_texture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &v);
                return;
        }
        if (m_format == GL_R32F)
        {
                GLfloat v = 0;
                glClearTexImage(m_texture, 0, GL_RED, GL_FLOAT, &v);
                return;
        }
        error("Unsupported clear texture format " + std::to_string(static_cast<long long>(m_format)));
}

// template <typename T>
// void Texture::image(T* pixels) const
//{
//        static_assert(is_float_buffer<T>);
//        const unsigned long long size = 4ull * m_width * m_height;
//        ASSERT(pixels->size() == size);
//        glGetTextureImage(m_texture, 0, GL_RGBA, GL_FLOAT, size * sizeof(GLfloat), pixels->data());
//}

// template <typename T>
// void Texture::sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, T* pixels) const
//{
//        ASSERT(width > 0 && height > 0 && width <= m_width && height <= m_height);
//        static_assert(is_float_buffer<T>);
//        const unsigned long long size = 4ull * width * height;
//        ASSERT(pixels->size() == size);
//        glGetTextureSubImage(m_texture, 0, xoffset, yoffset, 0, width, height, 1, GL_RGBA, GL_FLOAT, size * sizeof(GLfloat),
//                             pixels->data());
//}

int Texture::width() const
{
        return m_width;
}

int Texture::height() const
{
        return m_height;
}

GLenum Texture::format() const
{
        return m_format;
}

Texture::operator GLuint() const
{
        return m_texture;
}

//

DepthFramebuffer::DepthFramebuffer(GLenum depth_format, GLsizei width, GLsizei height) : m_depth(depth_format, width, height)
{
        glNamedFramebufferTexture(m_framebuffer, GL_DEPTH_ATTACHMENT, m_depth, 0);

        GLenum check = glCheckNamedFramebufferStatus(m_framebuffer, GL_FRAMEBUFFER);
        if (check != GL_FRAMEBUFFER_COMPLETE)
        {
                error("Error create shadow framebuffer: " + std::to_string(check));
        }
}

DepthFramebuffer::operator GLuint() const
{
        return m_framebuffer;
}

const Texture& DepthFramebuffer::texture() const
{
        return m_depth;
}

//

ColorFramebuffer::ColorFramebuffer(GLenum color_format, GLsizei width, GLsizei height) : m_color(color_format, width, height)
{
        constexpr GLenum COLOR_ATTACHMENT = GL_COLOR_ATTACHMENT0;

        glNamedFramebufferTexture(m_framebuffer, COLOR_ATTACHMENT, m_color, 0);

        GLenum check = glCheckNamedFramebufferStatus(m_framebuffer, GL_FRAMEBUFFER);
        if (check != GL_FRAMEBUFFER_COMPLETE)
        {
                error("Error create framebuffer: " + std::to_string(check));
        }

        glNamedFramebufferDrawBuffers(m_framebuffer, 1, &COLOR_ATTACHMENT);
}

ColorFramebuffer::operator GLuint() const
{
        return m_framebuffer;
}

const Texture& ColorFramebuffer::texture() const
{
        return m_color;
}

//

ColorDepthFramebufferMultisample::ColorDepthFramebufferMultisample(GLenum color_format, GLenum depth_format, GLsizei samples,
                                                                   GLsizei width, GLsizei height)
{
        constexpr GLenum COLOR_ATTACHMENT = GL_COLOR_ATTACHMENT0;
        constexpr GLenum DEPTH_ATTACHMENT = GL_DEPTH_ATTACHMENT;

        glNamedRenderbufferStorageMultisample(m_color, samples, color_format, width, height);
        glNamedRenderbufferStorageMultisample(m_depth, samples, depth_format, width, height);

        glNamedFramebufferRenderbuffer(m_framebuffer, COLOR_ATTACHMENT, GL_RENDERBUFFER, m_color);
        glNamedFramebufferRenderbuffer(m_framebuffer, DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

        GLenum check = glCheckNamedFramebufferStatus(m_framebuffer, GL_FRAMEBUFFER);
        if (check != GL_FRAMEBUFFER_COMPLETE)
        {
                error("Error create framebuffer multisample: " + std::to_string(check));
        }

        glNamedFramebufferDrawBuffers(m_framebuffer, 1, &COLOR_ATTACHMENT);
}

ColorDepthFramebufferMultisample::operator GLuint() const
{
        return m_framebuffer;
}

//

FramebufferBinder::FramebufferBinder(GLuint framebuffer)
        : m_draw_framebuffer(getIntegerValue(GL_DRAW_FRAMEBUFFER_BINDING)),
          m_read_framebuffer(getIntegerValue(GL_READ_FRAMEBUFFER_BINDING))
{
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

FramebufferBinder::~FramebufferBinder()
{
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_draw_framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_read_framebuffer);
}
}
