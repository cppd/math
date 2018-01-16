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

#include "com/colors_glsl.h"
#include "com/error.h"
#include "com/mat.h"
#include "com/type_detect.h"
#include "com/vec.h"
#include "opengl/opengl_functions.h"

#include <array>
#include <string>
#include <type_traits>
#include <vector>

constexpr int MAJOR_GL_VERSION = 4;
constexpr int MINOR_GL_VERSION = 5;
constexpr int ANTIALIASING_LEVEL = 4;
constexpr int DEPTH_BITS = 24;
constexpr int STENCIL_BITS = 8;
constexpr int RED_BITS = 8;
constexpr int GREEN_BITS = 8;
constexpr int BLUE_BITS = 8;
constexpr int ALPHA_BITS = 8;

inline std::vector<std::string> required_extensions()
{
        return std::vector<std::string>({"GL_ARB_bindless_texture", "GL_ARB_compute_variable_group_size"});
}

class Shader
{
        GLuint m_shader = 0;

protected:
        Shader(GLenum type, const std::string& shader_text)
        {
                m_shader = glCreateShader(type);
                try
                {
                        std::string source;
                        source = "#version " + std::to_string(MAJOR_GL_VERSION) + std::to_string(MINOR_GL_VERSION) +
                                 (MINOR_GL_VERSION < 10 ? "0" : "") + " core\n";
                        for (const std::string& ext : required_extensions())
                        {
                                source += "#extension " + ext + " : require\n";
                        }
                        source += glsl_color_space_functions();
                        source += "\n";
                        source += shader_text;
                        const char* const source_ptr = source.c_str();

                        glShaderSource(m_shader, 1, &source_ptr, nullptr);
                        glCompileShader(m_shader);

                        GLint status;
                        glGetShaderiv(m_shader, GL_COMPILE_STATUS, &status);
                        if (status != GL_TRUE)
                        {
                                GLint length;
                                glGetShaderiv(m_shader, GL_INFO_LOG_LENGTH, &length);
                                if (length > 1)
                                {
                                        std::vector<GLchar> buffer(length);
                                        glGetShaderInfoLog(m_shader, length, nullptr, buffer.data());
                                        error_source(std::string("CompileShader\n\n") + buffer.data(), source);
                                }
                                else
                                {
                                        error_source("CompileShader\n\nUnknown error", source);
                                }
                        }
                }
                catch (...)
                {
                        glDeleteShader(m_shader);
                        throw;
                }
        }

        ~Shader()
        {
                glDeleteShader(m_shader);
        }

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& from) noexcept
        {
                *this = std::move(from);
        }
        Shader& operator=(Shader&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteShader(m_shader);
                m_shader = from.m_shader;
                from.m_shader = 0;
                return *this;
        }

public:
        void attach_to_program(GLuint program) const noexcept
        {
                glAttachShader(program, m_shader);
        }
        void detach_from_program(GLuint program) const noexcept
        {
                glDetachShader(program, m_shader);
        }
};

class Program
{
        class AttachShader final
        {
                GLuint m_program;
                const Shader* m_shader;

        public:
                AttachShader(GLuint program, const Shader& shader) : m_program(program), m_shader(&shader)
                {
                        m_shader->attach_to_program(m_program);
                }
                ~AttachShader()
                {
                        if (m_shader && m_program)
                        {
                                m_shader->detach_from_program(m_program);
                        }
                }

                AttachShader(const AttachShader&) = delete;
                AttachShader& operator=(const AttachShader&) = delete;

                AttachShader(AttachShader&& from) noexcept
                {
                        *this = std::move(from);
                }
                AttachShader& operator=(AttachShader&& from) noexcept
                {
                        if (this == &from)
                        {
                                return *this;
                        }
                        m_program = from.m_program;
                        m_shader = from.m_shader;
                        from.m_program = 0;
                        from.m_shader = nullptr;
                        return *this;
                }
        };

        //
        GLuint m_program = 0;

        GLint get_uniform_location(const char* name) const
        {
                GLint loc = glGetUniformLocation(m_program, name);
                if (loc < 0)
                {
                        error(std::string("glGetUniformLocation error: ") + name);
                }
                return loc;
        }

protected:
        Program() = delete; // Программа без шейдеров не должна делаться

        template <typename... S>
        Program(const S&... shader)
        {
                static_assert(sizeof...(S) > 0);

                m_program = glCreateProgram();
                try
                {
                        std::vector<AttachShader> attaches;

                        (attaches.emplace_back(m_program, shader), ...);

                        glLinkProgram(m_program);

                        GLint status;
                        glGetProgramiv(m_program, GL_LINK_STATUS, &status);
                        if (status != GL_TRUE)
                        {
                                GLint length;
                                glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
                                if (length > 1)
                                {
                                        std::vector<GLchar> buffer(length);
                                        glGetProgramInfoLog(m_program, length, nullptr, buffer.data());
                                        error(std::string("LinkProgram Error: ") + buffer.data());
                                }
                                else
                                {
                                        error("LinkProgram Error");
                                }
                        }
                }
                catch (...)
                {
                        glDeleteProgram(m_program);
                        throw;
                }
        }

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program(Program&& from) noexcept
        {
                *this = std::move(from);
        }
        Program& operator=(Program&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteProgram(m_program);
                m_program = from.m_program;
                from.m_program = 0;
                return *this;
        }

        ~Program()
        {
                glDeleteProgram(m_program);
        }

        void use() const noexcept
        {
                glUseProgram(m_program);
        }

public:
        void set_uniform(const char* var_name, GLint var) const
        {
                glProgramUniform1i(m_program, get_uniform_location(var_name), var);
        }
        void set_uniform_unsigned(const char* var_name, GLuint var) const
        {
                glProgramUniform1ui(m_program, get_uniform_location(var_name), var);
        }
        void set_uniform(const char* var_name, GLfloat var) const
        {
                glProgramUniform1f(m_program, get_uniform_location(var_name), var);
        }
        void set_uniform(const char* var_name, GLdouble var) const
        {
                glProgramUniform1d(m_program, get_uniform_location(var_name), var);
        }

        void set_uniform(GLint loc, GLint var) const
        {
                glProgramUniform1i(m_program, loc, var);
        }
        void set_uniform_unsigned(GLint loc, GLuint var) const
        {
                glProgramUniform1ui(m_program, loc, var);
        }
        void set_uniform(GLint loc, GLfloat var) const
        {
                glProgramUniform1f(m_program, loc, var);
        }
        void set_uniform(GLint loc, GLdouble var) const
        {
                glProgramUniform1d(m_program, loc, var);
        }
        void set_uniform_handle(GLint loc, GLuint64 var) const
        {
                glProgramUniformHandleui64ARB(m_program, loc, var);
        }
        void set_uniform_handles(GLint loc, const std::vector<GLuint64>& var) const
        {
                glProgramUniformHandleui64vARB(m_program, loc, var.size(), var.data());
        }

        void set_uniform(const char* var_name, const vec2f& var) const
        {
                static_assert(sizeof(vec2f) == 2 * sizeof(float));
                glProgramUniform2fv(m_program, get_uniform_location(var_name), 1, var.data());
        }
        void set_uniform(const char* var_name, const vec3f& var) const
        {
                static_assert(sizeof(vec3f) == 3 * sizeof(float));
                glProgramUniform3fv(m_program, get_uniform_location(var_name), 1, var.data());
        }
        void set_uniform(const char* var_name, const vec4f& var) const
        {
                static_assert(sizeof(vec4f) == 4 * sizeof(float));
                glProgramUniform4fv(m_program, get_uniform_location(var_name), 1, var.data());
        }

        void set_uniform_float(const char* var_name, const Matrix<4, 4, double>& var) const
        {
                static_assert(sizeof(Matrix<4, 4, float>) == 16 * sizeof(float));
                static_assert(sizeof(Matrix<4, 4, double>) == 16 * sizeof(double));
                glProgramUniformMatrix4fv(m_program, get_uniform_location(var_name), 1, GL_TRUE, to_matrix<float>(var).data());
        }
        void set_uniform_float(const char* var_name, const Matrix<4, 4, float>& var) const
        {
                static_assert(sizeof(Matrix<4, 4, float>) == 16 * sizeof(float));
                glProgramUniformMatrix4fv(m_program, get_uniform_location(var_name), 1, GL_TRUE, var.data());
        }

        void set_uniform(const char* var_name, const std::vector<int>& var) const
        {
                glProgramUniform1iv(m_program, get_uniform_location(var_name), var.size(), var.data());
        }
        void set_uniform(const char* var_name, const std::vector<unsigned>& var) const
        {
                glProgramUniform1uiv(m_program, get_uniform_location(var_name), var.size(), var.data());
        }

        void set_uniform_handle(const char* var_name, GLuint64 var) const
        {
                glProgramUniformHandleui64ARB(m_program, get_uniform_location(var_name), var);
        }
        void set_uniform_handles(const char* var_name, const std::vector<GLuint64>& var) const
        {
                glProgramUniformHandleui64vARB(m_program, get_uniform_location(var_name), var.size(), var.data());
        }
};

class VertexShader final : public Shader
{
public:
        VertexShader(const std::string& shader_text) : Shader(GL_VERTEX_SHADER, shader_text)
        {
        }
};

class TessControlShader final : public Shader
{
public:
        TessControlShader(const std::string& shader_text) : Shader(GL_TESS_CONTROL_SHADER, shader_text)
        {
        }
};

class TessEvaluationShader final : public Shader
{
public:
        TessEvaluationShader(const std::string& shader_text) : Shader(GL_TESS_EVALUATION_SHADER, shader_text)
        {
        }
};

class GeometryShader final : public Shader
{
public:
        GeometryShader(const std::string& shader_text) : Shader(GL_GEOMETRY_SHADER, shader_text)
        {
        }
};

class FragmentShader final : public Shader
{
public:
        FragmentShader(const std::string& shader_text) : Shader(GL_FRAGMENT_SHADER, shader_text)
        {
        }
};

class ComputeShader final : public Shader
{
public:
        ComputeShader(const std::string& shader_text) : Shader(GL_COMPUTE_SHADER, shader_text)
        {
        }
};

class GraphicsProgram final : public Program
{
public:
        template <typename... S>
        GraphicsProgram(const S&... s) : Program(s...)
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
        ComputeProgram(const S&... s) : Program(s...)
        {
                static_assert((std::is_same_v<ComputeShader, S> && ...), "ComputeProgram accepts only compute shaders");
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

        GLuint64 get_texture_resident_handle() const noexcept
        {
                GLuint64 texture_handle = glGetTextureHandleARB(m_texture);
                glMakeTextureHandleResidentARB(texture_handle);
                return texture_handle;
        }
        GLuint64 get_image_resident_handle(GLint level, GLboolean layered, GLint layer, GLenum format, GLenum access) const
                noexcept
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

        int get_width() const noexcept
        {
                return m_width;
        }
        int get_height() const noexcept
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

class ShaderStorageBuffer final
{
        GLuint m_buffer = 0;

public:
        ShaderStorageBuffer() noexcept
        {
                glCreateBuffers(1, &m_buffer);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
        ~ShaderStorageBuffer()
        {
                glDeleteBuffers(1, &m_buffer);
        }
        ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
        ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
        ShaderStorageBuffer(ShaderStorageBuffer&& from) noexcept
        {
                *this = std::move(from);
        }
        ShaderStorageBuffer& operator=(ShaderStorageBuffer&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteBuffers(1, &m_buffer);
                m_buffer = from.m_buffer;
                from.m_buffer = 0;
                return *this;
        }

        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_static_draw(const T& data) const
        {
                glNamedBufferData(m_buffer, data.size() * sizeof(typename T::value_type), data.data(), GL_STATIC_DRAW);
        }
        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_static_copy(const T& data) const
        {
                glNamedBufferData(m_buffer, data.size() * sizeof(typename T::value_type), data.data(), GL_STATIC_COPY);
        }
        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_dynamic_draw(const T& data) const
        {
                glNamedBufferData(m_buffer, data.size() * sizeof(typename T::value_type), data.data(), GL_DYNAMIC_DRAW);
        }
        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_dynamic_copy(const T& data) const
        {
                glNamedBufferData(m_buffer, data.size() * sizeof(typename T::value_type), data.data(), GL_DYNAMIC_COPY);
        }

        void create_dynamic_copy(GLsizeiptr size) const noexcept
        {
                glNamedBufferData(m_buffer, size, nullptr, GL_DYNAMIC_COPY);
        }
        void create_static_copy(GLsizeiptr size) const noexcept
        {
                glNamedBufferData(m_buffer, size, nullptr, GL_STATIC_COPY);
        }

        template <typename T>
        void read(std::vector<T>* data) const
        {
                glGetNamedBufferSubData(m_buffer, 0, data->size() * sizeof(T), data->data());
        }

        void bind(GLuint binding_point) const noexcept
        {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, m_buffer);
        }
};

class ArrayBuffer final
{
        GLuint m_buffer = 0;

public:
        ArrayBuffer() noexcept
        {
                glCreateBuffers(1, &m_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        ~ArrayBuffer()
        {
                glDeleteBuffers(1, &m_buffer);
        }
        ArrayBuffer(const ArrayBuffer&) = delete;
        ArrayBuffer& operator=(const ArrayBuffer&) = delete;
        ArrayBuffer(ArrayBuffer&& from) noexcept
        {
                *this = std::move(from);
        }
        ArrayBuffer& operator=(ArrayBuffer&& from) noexcept
        {
                if (this == &from)
                {
                        return *this;
                }
                glDeleteBuffers(1, &m_buffer);
                m_buffer = from.m_buffer;
                from.m_buffer = 0;
                return *this;
        }

        void vertex_array_vertex_buffer(GLuint vertex_array, GLuint binding_index, GLintptr offset, GLsizei stride) const noexcept
        {
                glVertexArrayVertexBuffer(vertex_array, binding_index, m_buffer, offset, stride);
        }

        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_static_draw(const T& v) const
        {
                glNamedBufferData(m_buffer, v.size() * sizeof(typename T::value_type), v.data(), GL_STATIC_DRAW);
        }
        template <typename T>
        std::enable_if_t<IsVector<T> || IsArray<T>> load_dynamic_draw(const T& v) const
        {
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
        Texture2D m_texture;

        void set_parameters()
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

public:
        TextureRGBA32F(GLsizei width, GLsizei height, const std::vector<GLfloat>& pixels) noexcept
                : m_texture(1, GL_RGBA32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0 && (4ull * width * height == pixels.size()));

                m_texture.texture_sub_image_2d(0, 0, 0, width, height, GL_RGBA, GL_FLOAT, pixels.data());
                set_parameters();
        }

        TextureRGBA32F(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_RGBA32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0);

                set_parameters();
        }

        GLuint64 get_image_resident_handle_write_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_WRITE_ONLY);
        }
        GLuint64 get_image_resident_handle_read_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_READ_ONLY);
        }
        GLuint64 get_image_resident_handle_read_write() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_RGBA32F, GL_READ_WRITE);
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
                m_texture.copy_texture_sub_image_2d(0, 0, 0, 0, 0, m_texture.get_width(), m_texture.get_height());
        }

        void clear_tex_image(GLfloat r, GLfloat g, GLfloat b, GLfloat a) const noexcept
        {
                std::array<GLfloat, 4> v = {{r, g, b, a}};
                m_texture.clear_tex_image(0, GL_RGBA, GL_FLOAT, v.data());
        }
        void get_texture_image(std::vector<GLfloat>* pixels) const noexcept
        {
                unsigned long long size = 4ull * m_texture.get_width() * m_texture.get_height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RGBA, GL_FLOAT, size * sizeof(GLfloat), pixels->data());
        }
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   std::vector<GLfloat>* pixels) const noexcept
        {
                unsigned long long size = 4ull * width * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.get_width() && height <= m_texture.get_height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RGBA, GL_FLOAT,
                                                size * sizeof(GLfloat), pixels->data());
        }

        const Texture2D& get_texture() const noexcept
        {
                return m_texture;
        }
};

class TextureR32F final
{
        Texture2D m_texture;

        void set_parameters()
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

public:
        TextureR32F(GLsizei width, GLsizei height, const std::vector<GLfloat>& pixels) noexcept
                : m_texture(1, GL_R32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0 && static_cast<unsigned long long>(width) * height == pixels.size());

                m_texture.texture_sub_image_2d(0, 0, 0, width, height, GL_RED, GL_FLOAT, pixels.data());
                set_parameters();
        }

        TextureR32F(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_R32F, width, height)
        {
                ASSERT(width >= 0 && height >= 0);

                set_parameters();
        }

        GLuint64 get_image_resident_handle_write_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_WRITE_ONLY);
        }
        GLuint64 get_image_resident_handle_read_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_READ_ONLY);
        }
        GLuint64 get_image_resident_handle_read_write() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32F, GL_READ_WRITE);
        }

        void clear_tex_image(GLfloat v) const noexcept
        {
                m_texture.clear_tex_image(0, GL_RED, GL_FLOAT, &v);
        }
        void get_texture_image(std::vector<GLfloat>* pixels) const noexcept
        {
                unsigned long long size = m_texture.get_width() * m_texture.get_height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RED, GL_FLOAT, size * sizeof(GLfloat), pixels->data());
        }
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   std::vector<GLfloat>* pixels) const noexcept
        {
                unsigned long long size = width * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.get_width() && height <= m_texture.get_height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RED, GL_FLOAT,
                                                size * sizeof(GLfloat), pixels->data());
        }

        const Texture2D& get_texture() const noexcept
        {
                return m_texture;
        }
};

class TextureR32I final
{
        Texture2D m_texture;

public:
        TextureR32I(GLsizei width, GLsizei height) noexcept : m_texture(1, GL_R32I, width, height)
        {
                m_texture.texture_parameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_WRAP_T, GL_REPEAT);
                m_texture.texture_parameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                m_texture.texture_parameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }

        GLuint64 get_image_resident_handle_write_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_WRITE_ONLY);
        }
        GLuint64 get_image_resident_handle_read_only() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_READ_ONLY);
        }
        GLuint64 get_image_resident_handle_read_write() const noexcept
        {
                return m_texture.get_image_resident_handle(0, GL_FALSE, 0, GL_R32I, GL_READ_WRITE);
        }

        void clear_tex_image(GLint v) const noexcept
        {
                m_texture.clear_tex_image(0, GL_RED_INTEGER, GL_INT, &v);
        }
        void get_texture_image(std::vector<GLint>* pixels) const noexcept
        {
                unsigned long long size = m_texture.get_width() * m_texture.get_height();
                ASSERT(pixels->size() == size);
                m_texture.get_texture_image(0, GL_RED_INTEGER, GL_INT, size * sizeof(GLint), pixels->data());
        }
        void get_texture_sub_image(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, std::vector<GLint>* pixels) const
                noexcept
        {
                unsigned long long size = width * height;
                ASSERT(pixels->size() == size);
                ASSERT(width > 0 && height > 0);
                ASSERT(width <= m_texture.get_width() && height <= m_texture.get_height());
                m_texture.get_texture_sub_image(0, xoffset, yoffset, 0, width, height, 1, GL_RED_INTEGER, GL_INT,
                                                size * sizeof(GLint), pixels->data());
        }

        const Texture2D& get_texture() const noexcept
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

        const Texture2D& get_texture() const noexcept
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
                m_fb.named_framebuffer_texture(GL_DEPTH_ATTACHMENT, m_depth.get_texture(), 0);

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

        const TextureDepth32& get_depth_texture() const noexcept
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
                m_fb.named_framebuffer_texture(GL_COLOR_ATTACHMENT0, m_color.get_texture(), 0);
                m_fb.named_framebuffer_texture(GL_DEPTH_ATTACHMENT, m_depth.get_texture(), 0);

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

        const TextureRGBA32F& get_color_texture() const noexcept
        {
                return m_color;
        }
};
