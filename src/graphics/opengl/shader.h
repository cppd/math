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

#if defined(OPENGL_FOUND)

#include "graphics/opengl/functions.h"
#include "graphics/opengl/objects.h"

#include <string_view>
#include <type_traits>
#include <vector>

namespace opengl
{
class Shader
{
        ShaderHandle m_shader;

protected:
        Shader(GLenum type, const std::string_view& shader_text);
        ~Shader();

public:
        void attach_to_program(GLuint program) const;
        void detach_from_program(GLuint program) const;
};

struct VertexShader final : Shader
{
        VertexShader(const std::string_view& text);
};

struct TessControlShader final : Shader
{
        TessControlShader(const std::string_view& text);
};

struct TessEvaluationShader final : Shader
{
        TessEvaluationShader(const std::string_view& text);
};

struct GeometryShader final : Shader
{
        GeometryShader(const std::string_view& text);
};

struct FragmentShader final : Shader
{
        FragmentShader(const std::string_view& text);
};

struct ComputeShader final : Shader
{
        ComputeShader(const std::string_view& text);
};

class Program
{
        ProgramHandle m_program;

protected:
        Program(const std::vector<const Shader*>& shaders);
        Program(Program&&) = default;
        ~Program();

        void use() const;

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
                                std::is_same_v<FragmentShader, S>)&&...));
        }

        GraphicsProgram(GraphicsProgram&& program) : Program(std::move(program))
        {
        }

        void draw_arrays(GLenum mode, GLint first, GLsizei count) const
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
                static_assert((std::is_same_v<ComputeShader, S> && ...));
        }

        ComputeProgram(ComputeProgram&& program) : Program(std::move(program))
        {
        }

        void dispatch_compute(unsigned num_groups_x, unsigned num_groups_y, unsigned num_groups_z) const
        {
                Program::use();
                glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
        }

        void dispatch_compute(unsigned num_groups_x, unsigned num_groups_y, unsigned num_groups_z, unsigned group_size_x,
                              unsigned group_size_y, unsigned group_size_z) const
        {
                Program::use();
                glDispatchComputeGroupSizeARB(num_groups_x, num_groups_y, num_groups_z, group_size_x, group_size_y, group_size_z);
        }
};
}

#endif
