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

#include "shader.h"

#include "com/error.h"

#include <algorithm>
#include <array>

// clang-format off
constexpr std::string_view GLSL_HEADER =
{
#include "header.glsl.str"
};
// clang-format on

constexpr std::string_view EMPTY_LINE = "\n";

namespace
{
template <typename Type, size_t size>
constexpr Type to_type()
{
        constexpr bool is_same = std::is_same_v<size_t, std::remove_cv_t<Type>>;
        static_assert(is_same || std::numeric_limits<Type>::is_specialized);
        static_assert(is_same || size <= std::numeric_limits<Type>::max());
        return size;
}

template <typename Type>
constexpr Type to_type(size_t size)
{
        constexpr bool is_same = std::is_same_v<size_t, std::remove_cv_t<Type>>;
        static_assert(is_same || std::numeric_limits<Type>::is_specialized);
        if constexpr (!is_same)
        {
                ASSERT(size <= std::numeric_limits<Type>::max());
        }
        return size;
}

template <typename Data, typename Size, size_t N>
std::string string_source(const std::array<const Data*, N>& pointers, const std::array<Size, N>& sizes)
{
        std::string s;
        for (size_t i = 0; i < N; ++i)
        {
                s += std::string_view(pointers[i], sizes[i]);
        }
        return s;
}

GLint get_uniform_location(GLuint program, const char* name)
{
        GLint loc = glGetUniformLocation(program, name);
        if (loc < 0)
        {
                error(std::string("glGetUniformLocation error: ") + name);
        }
        return loc;
}

class AttachShader
{
        GLuint m_program = 0;
        const opengl::Shader* m_shader = nullptr;

        void destroy() noexcept
        {
                if (m_program && m_shader)
                {
                        m_shader->detach_from_program(m_program);
                }
        }

        void move(AttachShader* from) noexcept
        {
                m_program = from->m_program;
                m_shader = from->m_shader;
                from->m_program = 0;
                from->m_shader = nullptr;
        }

public:
        AttachShader(GLuint program, const opengl::Shader* shader) : m_program(program), m_shader(shader)
        {
                m_shader->attach_to_program(m_program);
        }

        ~AttachShader()
        {
                destroy();
        }

        AttachShader(const AttachShader&) = delete;
        AttachShader& operator=(const AttachShader&) = delete;

        AttachShader(AttachShader&& from) noexcept
        {
                move(&from);
        }
#if 0
        AttachShader& operator=(AttachShader&& from) noexcept
        {
                if (this != &from)
                {
                        destroy();
                        move(&from);
                }
                return *this;
        }
#endif
};
}

namespace opengl
{
Shader::Shader(GLenum type, const std::string_view& shader_text)
{
        m_shader = glCreateShader(type);
        try
        {
                const std::array<const GLchar*, 3> source_pointers = {GLSL_HEADER.data(), EMPTY_LINE.data(), shader_text.data()};

                const std::array<GLint, 3> source_sizes = {to_type<GLint, GLSL_HEADER.size()>(),
                                                           to_type<GLint, EMPTY_LINE.size()>(),
                                                           to_type<GLint>(shader_text.size())};

                glShaderSource(m_shader, source_pointers.size(), source_pointers.data(), source_sizes.data());

                glCompileShader(m_shader);

                GLint status;
                glGetShaderiv(m_shader, GL_COMPILE_STATUS, &status);
                if (status != GL_TRUE)
                {
                        std::string error_message = "CompileShader\n\n";

                        GLint length;
                        glGetShaderiv(m_shader, GL_INFO_LOG_LENGTH, &length);
                        if (length > 1)
                        {
                                std::vector<GLchar> buffer(length);
                                glGetShaderInfoLog(m_shader, length, nullptr, buffer.data());
                                error_message += buffer.data();
                        }
                        else
                        {
                                error_message += "Unknown error";
                        }

                        error_source(std::move(error_message), string_source(source_pointers, source_sizes));
                }
        }
        catch (...)
        {
                glDeleteShader(m_shader);
                throw;
        }
}

Shader::~Shader()
{
        glDeleteShader(m_shader);
}

Shader::Shader(Shader&& from) noexcept
{
        *this = std::move(from);
}

Shader& Shader::operator=(Shader&& from) noexcept
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

void Shader::attach_to_program(GLuint program) const noexcept
{
        glAttachShader(program, m_shader);
}

void Shader::detach_from_program(GLuint program) const noexcept
{
        glDetachShader(program, m_shader);
}

//

Program::Program(const std::vector<const Shader*>& shaders)
{
        ASSERT(shaders.size() > 0);
        ASSERT(std::all_of(shaders.cbegin(), shaders.cend(), [](const Shader* s) { return s != nullptr; }));

        m_program = glCreateProgram();
        try
        {
                std::vector<AttachShader> attaches;
                for (const Shader* s : shaders)
                {
                        attaches.emplace_back(m_program, s);
                }

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

Program::Program(Program&& from) noexcept
{
        *this = std::move(from);
}

Program& Program::operator=(Program&& from) noexcept
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

Program::~Program()
{
        glDeleteProgram(m_program);
}

void Program::use() const noexcept
{
        glUseProgram(m_program);
}

void Program::set_uniform_handle(GLint loc, GLuint64 var) const
{
        glProgramUniformHandleui64ARB(m_program, loc, var);
}

void Program::set_uniform_handles(GLint loc, const std::vector<GLuint64>& var) const
{
        glProgramUniformHandleui64vARB(m_program, loc, var.size(), var.data());
}

void Program::set_uniform_handle(const char* var_name, GLuint64 var) const
{
        set_uniform_handle(get_uniform_location(m_program, var_name), var);
}

void Program::set_uniform_handles(const char* var_name, const std::vector<GLuint64>& var) const
{
        set_uniform_handles(get_uniform_location(m_program, var_name), var);
}
}
