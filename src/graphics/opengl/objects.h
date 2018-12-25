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

#pragma once

#include "com/color/conversion_span.h"
#include "com/error.h"
#include "com/matrix.h"
#include "com/type_detect.h"
#include "com/vec.h"
#include "graphics/opengl/functions/opengl_functions.h"

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace opengl
{
class Shader
{
        GLuint m_shader = 0;

protected:
        Shader(GLenum type, const std::string_view& shader_text);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& from) noexcept;
        Shader& operator=(Shader&& from) noexcept;

public:
        void attach_to_program(GLuint program) const noexcept;
        void detach_from_program(GLuint program) const noexcept;
};

class Program
{
        GLuint m_program = 0;

protected:
        Program(const std::vector<const Shader*>& shaders);
        ~Program();

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;

        Program(Program&& from) noexcept;
        Program& operator=(Program&& from) noexcept;

        void use() const noexcept;

public:
        void set_uniform_handle(GLint loc, GLuint64 var) const;
        void set_uniform_handles(GLint loc, const std::vector<GLuint64>& var) const;
        void set_uniform_handle(const char* var_name, GLuint64 var) const;
        void set_uniform_handles(const char* var_name, const std::vector<GLuint64>& var) const;
};

class VertexShader final : public Shader
{
public:
        VertexShader(const std::string_view& shader_text) : Shader(GL_VERTEX_SHADER, shader_text)
        {
        }
};

class TessControlShader final : public Shader
{
public:
        TessControlShader(const std::string_view& shader_text) : Shader(GL_TESS_CONTROL_SHADER, shader_text)
        {
        }
};

class TessEvaluationShader final : public Shader
{
public:
        TessEvaluationShader(const std::string_view& shader_text) : Shader(GL_TESS_EVALUATION_SHADER, shader_text)
        {
        }
};

class GeometryShader final : public Shader
{
public:
        GeometryShader(const std::string_view& shader_text) : Shader(GL_GEOMETRY_SHADER, shader_text)
        {
        }
};

class FragmentShader final : public Shader
{
public:
        FragmentShader(const std::string_view& shader_text) : Shader(GL_FRAGMENT_SHADER, shader_text)
        {
        }
};

class ComputeShader final : public Shader
{
public:
        ComputeShader(const std::string_view& shader_text) : Shader(GL_COMPUTE_SHADER, shader_text)
        {
        }
};

class GraphicsProgram final : public Program
{
public:
        template <typename... S>
        GraphicsProgram(const S&... s) : Program({&s...})
        {
                static_assert(((std::is_same_v<VertexShader, S> || std::is_same_v<TessControlShader, S> ||
                                std::is_same_v<TessEvaluationShader, S> || std::is_same_v<GeometryShader, S> ||
                                std::is_same_v<FragmentShader, S>)&&...),
                              "GraphicsProgram accepts only vertex, tesselation, geometry and fragment shaders");
        }

        void draw_arrays(GLenum mode, GLint first, GLsizei count) const noexcept
        {
                Program::use();
                glDrawArrays(mode, first, count);
        }
};

class ComputeProgram final : public Program
{
public:
        template <typename... S>
        ComputeProgram(const S&... s) : Program({&s...})
        {
                static_assert((std::is_same_v<ComputeShader, S> && ...), "ComputeProgram accepts only compute shaders");
        }

        void dispatch_compute(unsigned num_groups_x, unsigned num_groups_y, unsigned num_groups_z) const noexcept
        {
                Program::use();
                glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        }

        void dispatch_compute(unsigned num_groups_x, unsigned num_groups_y, unsigned num_groups_z, unsigned group_size_x,
                              unsigned group_size_y, unsigned group_size_z) const noexcept
        {
                Program::use();
                glDispatchComputeGroupSizeARB(num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z);
        }
};

class Texture2D final
{
        class Texture2DHandle final
        {
                GLuint m_texture = 0;

        public:
                Texture2DHandle() noexcept
                {
                        glCreateTextures(GL_TEXTURE_2D, 1, &m_texture);
                        glBindTexture(GL_TEXTURE_2D, m_texture);
                        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        glBindTexture(GL_TEXTURE_2D, 0);
                }
                ~Texture2DHandle()
                {
                        glDeleteTextures(1, &m_texture);
                }
                Texture2DHandle(const Texture2DHandle&) = delete;
                Texture2DHandle& operator=(const Texture2DHandle&) = delete;
                Texture2DHandle(Texture2DHandle&& from) noexcept
                {
                        *this = std::move(from);
                }
                Texture2DHandle& operator=(Texture2DHandle&& from) noexcept
                {
                        if (this == &from)
                        {
                                return *this;
                        }
                        glDeleteTextures(1, &m_texture);
                        m_texture = from.m_texture;
                        from.m_texture = 0;
                        return *this;
                }
                operator GLuint() const noexcept
                {
                        return m_texture;
                }
        };

        Texture2DHandle m_texture;
        int m_width = 0, m_height = 0;

public:
        Texture2D(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) noexcept
        {
                glTextureStorage2D(m_texture, levels, internalformat, width, height);
                m_width = width;
                m_height = height;
        }

        void texture_sub_image_2d(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
                                  GLenum type, const void* pixels) const noexcept
        {
                glTextureSubImage2D(m_texture, level, xoffset, yoffset, width, height, format, type, pixels);
        }

        void copy_texture_sub_image_2d(GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width,
                                       GLsizei height) const noexcept
        {
                glCopyTextureSubImage2D(m_texture, level, xoffset, yoffset, x, y, width, height);
        }

        void texture_parameter(GLenum pname, GLint param) const noexcept
        {
                glTextureParameteri(m_texture, pname, param);
        }
        void texture_parameter(GLenum pname, GLfloat param) const noexcept
        {
                glTextureParameterf(m_texture, pname, param);
        }

        void bind_image_texture(GLuint unit, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const
                noexcept
        {
                glBindImageTexture(unit, m_texture, level, layered, layer, access, format);
        }

        GLuint64 texture_resident_handle() const noexcept
        {
                GLuint64 texture_handle = glGetTextureHandleARB(m_texture);
                glMakeTextureHandleResidentARB(texture_handle);
                return texture_handle;
        }
        GLuint64 image_resident_handle(GLint level, GLboolean layered, GLint layer, GLenum format, GLenum access) const noexcept
        {
                GLuint64 image_handle = glGetImageHandleARB(m_texture, level, layered, layer, format);
                glMakeImageHandleResidentARB(image_handle, access);
                return image_handle;
        }

        void clear_tex_image(GLint level, GLenum format, GLenum type, const void* data) const noexcept
        {
                glClearTexImage(m_texture, level, format, type, data);
        }
        void get_texture_image(GLint level, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const noexcept
        {
                glGetTextureImage(m_texture, level, format, type, bufSize, pixels);
        }
        void get_texture_sub_image(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height,
                                   GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void* pixels) const noexcept
        {
                glGetTextureSubImage(m_texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize,
                                     pixels);
        }

        void named_framebuffer_texture(GLuint framebuffer, GLenum attachment, GLint level) const noexcept
        {
                glNamedFramebufferTexture(framebuffer, attachment, m_texture, level);
        }

        int width() const noexcept
        {
                return m_width;
        }
        int height() const noexcept
        {
                return m_height;
        }
};

class FrameBuffer final
{
        GLuint m_framebuffer = 0;

public:
        FrameBuffer() noexcept
        {
                glCreateFramebuffers(1, &m_framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        ~FrameBuffer()
        {
                glDeleteFramebuffers(1, &m_framebuffer);
        }
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer&) = delete;
        FrameBuffer(FrameBuffer&& from) noexcept
        {
                *this = std::move(from);
        }
        FrameBuffer& operator=(FrameBuffer&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteFramebuffers(1, &m_framebuffer);
                m_framebuffer = from.m_framebuffer;
                from.m_framebuffer = 0;
                return *this;
        }

        GLenum check_named_framebuffer_status() const noexcept
        {
                return glCheckNamedFramebufferStatus(m_framebuffer, GL_FRAMEBUFFER);
        }

        void bind_framebuffer() const noexcept
        {
                glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        }
        void unbind_framebuffer() const noexcept
        {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void named_framebuffer_draw_buffer(GLenum buf) const noexcept
        {
                glNamedFramebufferDrawBuffer(m_framebuffer, buf);
        }
        void named_framebuffer_draw_buffers(GLsizei n, const GLenum* bufs) const noexcept
        {
                glNamedFramebufferDrawBuffers(m_framebuffer, n, bufs);
        }

        void named_framebuffer_texture(GLenum attachment, const Texture2D& texture, GLint level) const noexcept
        {
                texture.named_framebuffer_texture(m_framebuffer, attachment, level);
        }
};

class Buffer final
{
        GLuint m_buffer = 0;

        void destroy() noexcept
        {
                if (m_buffer != 0)
                {
                        glDeleteBuffers(1, &m_buffer);
                }
        }

        void move(Buffer* from) noexcept
        {
                m_buffer = from->m_buffer;
                from->m_buffer = 0;
        }

public:
        Buffer(GLenum target)
        {
                glCreateBuffers(1, &m_buffer);
                glBindBuffer(target, m_buffer);
                glBindBuffer(target, 0);
        }
        ~Buffer()
        {
                destroy();
        }

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        Buffer(Buffer&& from) noexcept
        {
                move(&from);
        }
        Buffer& operator=(Buffer&& from) noexcept
        {
                if (this != &from)
                {
                        destroy();
                        move(&from);
                }
                return *this;
        }

        operator GLuint() const noexcept
        {
                return m_buffer;
        }
};

class UniformBuffer final
{
        Buffer m_buffer;
        GLsizeiptr m_data_size;

        void copy(GLintptr offset, const void* data, GLsizeiptr data_size) const noexcept
        {
                ASSERT(offset + data_size <= m_data_size);

                void* map_memory_data =
                        glMapNamedBufferRange(m_buffer, offset, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

                std::memcpy(map_memory_data, data, data_size);

                glUnmapNamedBuffer(m_buffer);
        }

public:
        UniformBuffer(GLsizeiptr data_size) noexcept : m_buffer(GL_UNIFORM_BUFFER), m_data_size(data_size)
        {
                glNamedBufferStorage(m_buffer, data_size, nullptr, GL_MAP_WRITE_BIT);
        }

        void bind(GLuint point) const noexcept
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, point, m_buffer);
        }

        GLsizeiptr size() const noexcept
        {
                return m_data_size;
        }

        template <typename T>
        void copy(GLintptr offset, const T& data) const noexcept
        {
                copy(offset, &data, sizeof(data));
        }
        template <typename T>
        void copy(GLintptr offset, GLintptr data_offset, size_t data_size, const T& data) const noexcept
        {
                ASSERT(data_offset + data_size <= sizeof(data));

                copy(offset, data_offset + reinterpret_cast<const char*>(&data), data_size);
        }
        template <typename T>
        void copy(const T& data) const noexcept
        {
                ASSERT(size() == sizeof(data));

                copy(0, &data, sizeof(data));
        }
};

class StorageBuffer final
{
        Buffer m_buffer;
        GLsizeiptr m_data_size;

        void copy_to(GLintptr offset, const void* data, GLsizeiptr data_size) const noexcept
        {
                ASSERT(offset + data_size <= m_data_size);

                void* map_memory_data =
                        glMapNamedBufferRange(m_buffer, offset, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

                std::memcpy(map_memory_data, data, data_size);

                glUnmapNamedBuffer(m_buffer);
        }

        void copy_from(GLintptr offset, void* data, GLsizeiptr data_size) const noexcept
        {
                ASSERT(offset + data_size <= m_data_size);

                void* map_memory_data = glMapNamedBufferRange(m_buffer, offset, data_size, GL_MAP_READ_BIT);

                std::memcpy(data, map_memory_data, data_size);

                glUnmapNamedBuffer(m_buffer);
        }

        template <typename T>
        static std::enable_if_t<is_vector<T> || is_array<T>, size_t> binary_size(const T& c) noexcept
        {
                return c.size() * sizeof(typename T::value_type);
        }

public:
        StorageBuffer(GLsizeiptr data_size) noexcept : m_buffer(GL_SHADER_STORAGE_BUFFER), m_data_size(data_size)
        {
                glNamedBufferStorage(m_buffer, data_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
        }

        template <typename T, typename = std::enable_if_t<sizeof(std::declval<T>().size()) && sizeof(typename T::value_type)>>
        StorageBuffer(const T& data) noexcept : StorageBuffer(binary_size(data))
        {
                write(data);
        }

        void bind(GLuint point) const noexcept
        {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, point, m_buffer);
        }

        GLsizeiptr size() const noexcept
        {
                return m_data_size;
        }

        template <typename T>
        void write(const T& data) const noexcept
        {
                static_assert(is_vector<T> || is_array<T>);
                copy_to(0, data.data(), binary_size(data));
        }

        template <typename T>
        std::enable_if_t<is_vector<T> || is_array<T>> read(T* data) const noexcept
        {
                copy_from(0, data->data(), binary_size(*data));
        }
        template <typename T>
        std::enable_if_t<!is_vector<T> && !is_array<T>> read(T* data) const noexcept
        {
                copy_from(0, data, sizeof(T));
        }
};

class ArrayBuffer final
{
        Buffer m_buffer = 0;

public:
        ArrayBuffer() noexcept : m_buffer(GL_ARRAY_BUFFER)
        {
        }

        void vertex_array_vertex_buffer(GLuint vertex_array, GLuint binding_index, GLintptr offset, GLsizei stride) const noexcept
        {
                glVertexArrayVertexBuffer(vertex_array, binding_index, m_buffer, offset, stride);
        }

        template <typename T>
        void load_static_draw(const T& v) const
        {
                static_assert(is_vector<T> || is_array<T>);
                glNamedBufferData(m_buffer, v.size() * sizeof(typename T::value_type), v.data(), GL_STATIC_DRAW);
        }
        template <typename T>
        void load_dynamic_draw(const T& v) const
        {
                static_assert(is_vector<T> || is_array<T>);
                glNamedBufferData(m_buffer, v.size() * sizeof(typename T::value_type), v.data(), GL_DYNAMIC_DRAW);
        }
};

class VertexArray final
{
        GLuint m_vertex_array = 0;

public:
        VertexArray() noexcept
        {
                glCreateVertexArrays(1, &m_vertex_array);
        }
        ~VertexArray()
        {
                glDeleteVertexArrays(1, &m_vertex_array);
        }
        VertexArray(const VertexArray&) = delete;
        VertexArray& operator=(const VertexArray&) = delete;
        VertexArray(VertexArray&& from) noexcept
        {
                *this = std::move(from);
        }
        VertexArray& operator=(VertexArray&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteVertexArrays(1, &m_vertex_array);
                m_vertex_array = from.m_vertex_array;
                from.m_vertex_array = 0;
                return *this;
        }

        void bind() const noexcept
        {
                glBindVertexArray(m_vertex_array);
        }

        void attrib_pointer(GLuint attrib_index, GLint size, GLenum type, const ArrayBuffer& buffer, GLintptr offset,
                            GLsizei stride, bool enable) const noexcept
        {
                GLuint binding_index = attrib_index;
                glVertexArrayAttribFormat(m_vertex_array, attrib_index, size, type, GL_FALSE, 0);
                glVertexArrayAttribBinding(m_vertex_array, attrib_index, binding_index);
                buffer.vertex_array_vertex_buffer(m_vertex_array, binding_index, offset, stride);
                if (enable)
                {
                        glEnableVertexArrayAttrib(m_vertex_array, attrib_index);
                }
        }
        void attrib_i_pointer(GLuint attrib_index, GLint size, GLenum type, const ArrayBuffer& buffer, GLintptr offset,
                              GLsizei stride, bool enable) const noexcept
        {
                GLuint binding_index = attrib_index;
                glVertexArrayAttribIFormat(m_vertex_array, attrib_index, size, type, 0);
                glVertexArrayAttribBinding(m_vertex_array, attrib_index, binding_index);
                buffer.vertex_array_vertex_buffer(m_vertex_array, binding_index, offset, stride);
                if (enable)
                {
                        glEnableVertexArrayAttrib(m_vertex_array, attrib_index);
                }
        }
        void enable_attrib(GLuint index) const noexcept
        {
                glEnableVertexArrayAttrib(m_vertex_array, index);
        }
};

class TextureRGBA32F final
{
        template <typename T>
        static constexpr bool is_float_buffer = (is_vector<T> || is_array<T>)&&std::is_same_v<typename T::value_type, GLfloat>;

        Texture2D m_texture;

        void set_parameters()
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

public:
        TextureRGBA32F(GLsizei width, GLsizei height, const Span<const std::uint_least8_t>& srgb_uint8_rgba_pixels) noexcept
                : m_texture(1, GL_RGBA32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0 && (4ull * width * height == srgb_uint8_rgba_pixels.size()));

                std::vector<float> buffer = color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_float(srgb_uint8_rgba_pixels);

                m_texture.texture_sub_image_2d(0, 0, 0, width, height, GL_RGBA, GL_FLOAT, buffer.data());
                set_parameters();
        }

        TextureRGBA32F(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_RGBA32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0);

                set_parameters();
        }

        GLuint64 image_resident_handle_write_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_WRITE_ONLY);
        }
        GLuint64 image_resident_handle_read_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_READ_ONLY);
        }
        GLuint64 image_resident_handle_read_write() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_READ_WRITE);
        }

        void bind_image_texture_read_only(GLuint unit) const noexcept
        {
                m_texture.bind_image_texture(unit, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        }
        void bind_image_texture_write_only(GLuint unit) const noexcept
        {
                m_texture.bind_image_texture(unit, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        }
        void bind_image_texture_read_write(GLuint unit) const noexcept
        {
                m_texture.bind_image_texture(unit, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        }

        void copy_texture_sub_image() const noexcept
        {
                m_texture.copy_texture_sub_image_2d(0, 0, 0, 0, 0, m_texture.width(), m_texture.height());
        }

        void clear_tex_image(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        {
                std::array<GLfloat, 4> v = {r, g, b, a};
                m_texture.clear_tex_image(0, GL_RGBA, GL_FLOAT, v.data());
        }
        template <typename T>
        void get_texture_image(T* pixels) const noexcept
        {
                static_assert(is_float_buffer<T>);
                unsigned long long size = 4ull * m_texture.width() * m_texture.height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RGBA, GL_FLOAT, size * sizeof(GLfloat), pixels->data());
        }
        template <typename T>
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, T* pixels) const noexcept
        {
                static_assert(is_float_buffer<T>);
                unsigned long long size = 4ull * width * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.width() && height <= m_texture.height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RGBA, GL_FLOAT,
                                                size * sizeof(GLfloat), pixels->data());
        }

        const Texture2D& texture() const noexcept
        {
                return m_texture;
        }
};

class TextureR32F final
{
        template <typename T>
        static constexpr bool is_float_buffer = (is_vector<T> || is_array<T>)&&std::is_same_v<typename T::value_type, GLfloat>;

        Texture2D m_texture;

        void set_parameters()
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

public:
        TextureR32F(GLsizei width, GLsizei height, const Span<const std::uint_least8_t>& srgb_uint8_grayscale_pixels) noexcept
                : m_texture(1, GL_R32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0 && 1ull * width * height == srgb_uint8_grayscale_pixels.size());

                std::vector<float> buffer =
                        color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_float(srgb_uint8_grayscale_pixels);

                m_texture.texture_sub_image_2d(0, 0, 0, width, height, GL_RED, GL_FLOAT, buffer.data());
                set_parameters();
        }

        TextureR32F(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_R32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0);

                set_parameters();
        }

        GLuint64 image_resident_handle_write_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_WRITE_ONLY);
        }
        GLuint64 image_resident_handle_read_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_READ_ONLY);
        }
        GLuint64 image_resident_handle_read_write() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_READ_WRITE);
        }

        void clear_tex_image(GLfloat v) const noexcept
        {
                m_texture.clear_tex_image(0, GL_RED, GL_FLOAT, &v);
        }
        template <typename T>
        void get_texture_image(T* pixels) const noexcept
        {
                static_assert(is_float_buffer<T>);
                unsigned long long size = static_cast<unsigned long long>(m_texture.width()) * m_texture.height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RED, GL_FLOAT, size * sizeof(GLfloat), pixels->data());
        }
        template <typename T>
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, T* pixels) const noexcept
        {
                static_assert(is_float_buffer<T>);
                unsigned long long size = static_cast<unsigned long long>(width) * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.width() && height <= m_texture.height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RED, GL_FLOAT,
                                                size * sizeof(GLfloat), pixels->data());
        }

        const Texture2D& texture() const noexcept
        {
                return m_texture;
        }
};

class TextureR32I final
{
        template <typename T>
        static constexpr bool is_int_buffer = (is_vector<T> || is_array<T>)&&std::is_same_v<typename T::value_type, GLint>;

        Texture2D m_texture;

public:
        TextureR32I(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_R32I, width, height)
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        GLuint64 image_resident_handle_write_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_WRITE_ONLY);
        }
        GLuint64 image_resident_handle_read_only() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_READ_ONLY);
        }
        GLuint64 image_resident_handle_read_write() const noexcept
        {
                return m_texture.image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_READ_WRITE);
        }

        void clear_tex_image(GLint v) const noexcept
        {
                m_texture.clear_tex_image(0, GL_RED_INTEGER, GL_INT, &v);
        }
        template <typename T>
        void get_texture_image(T* pixels) const noexcept
        {
                static_assert(is_int_buffer<T>);
                unsigned long long size = static_cast<unsigned long long>(m_texture.width()) * m_texture.height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RED_INTEGER, GL_INT, size * sizeof(GLint), pixels->data());
        }
        template <typename T>
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, T* pixels) const noexcept
        {
                static_assert(is_int_buffer<T>);
                unsigned long long size = static_cast<unsigned long long>(width) * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.width() && height <= m_texture.height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RED_INTEGER, GL_INT,
                                                size * sizeof(GLint), pixels->data());
        }

        const Texture2D& texture() const noexcept
        {
                return m_texture;
        }
};

class TextureDepth32 final
{
        Texture2D m_texture;

public:
        TextureDepth32(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_DEPTH_COMPONENT32, width, height)
        {
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                m_texture.texture_parameter(GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        const Texture2D& texture() const noexcept
        {
                return m_texture;
        }
};

class ShadowBuffer final
{
        FrameBuffer m_fb;
        TextureDepth32 m_depth;

public:
        ShadowBuffer(GLsizei width, GLsizei height) : m_depth(width, height)
        {
                m_fb.named_framebuffer_texture(GL_DEPTH_ATTACHMENT, m_depth.texture(), 0);

                GLenum check = m_fb.check_named_framebuffer_status();
                if (check != GL_FRAMEBUFFER_COMPLETE)
                {
                        error("Error create shadow framebuffer: " + std::to_string(check));
                }
        }

        void bind_buffer() const noexcept
        {
                m_fb.bind_framebuffer();
        }
        void unbind_buffer() const noexcept
        {
                m_fb.unbind_framebuffer();
        }

        const TextureDepth32& depth_texture() const noexcept
        {
                return m_depth;
        }
};

class ColorBuffer final
{
        FrameBuffer m_fb;
        TextureRGBA32F m_color;
        TextureDepth32 m_depth;

public:
        ColorBuffer(GLsizei width, GLsizei height) : m_color(width, height), m_depth(width, height)
        {
                m_fb.named_framebuffer_texture(GL_COLOR_ATTACHMENT0, m_color.texture(), 0);
                m_fb.named_framebuffer_texture(GL_DEPTH_ATTACHMENT, m_depth.texture(), 0);

                GLenum check = m_fb.check_named_framebuffer_status();
                if (check != GL_FRAMEBUFFER_COMPLETE)
                {
                        error("Error create framebuffer: " + std::to_string(check));
                }

                const GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
                m_fb.named_framebuffer_draw_buffers(1, draw_buffers);
        }

        void bind_buffer() const noexcept
        {
                m_fb.bind_framebuffer();
        }
        void unbind_buffer() const noexcept
        {
                m_fb.unbind_framebuffer();
        }

        const TextureRGBA32F& color_texture() const noexcept
        {
                return m_color;
        }
};

template <GLenum... E>
class GLEnableAndRestore
{
        static_assert(sizeof...(E) > 0);
        std::array<bool, sizeof...(E)> m_enabled;

        static bool get_errors()
        {
                while (glGetError() != GL_NO_ERROR)
                {
                }
                return true;
        }

public:
        GLEnableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] = glIsEnabled(E), glEnable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
        ~GLEnableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] ? glEnable(E) : glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
};

template <GLenum... E>
class GLDisableAndRestore
{
        static_assert(sizeof...(E) > 0);
        std::array<bool, sizeof...(E)> m_enabled;

        static bool get_errors()
        {
                while (glGetError() != GL_NO_ERROR)
                {
                }
                return true;
        }

public:
        GLDisableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] = glIsEnabled(E), glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
        ~GLDisableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] ? glEnable(E) : glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
};
}
