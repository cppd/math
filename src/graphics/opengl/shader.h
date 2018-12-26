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

#include "graphics/opengl/functions/opengl_functions.h"

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

//

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
}
