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

#include "shader_memory.h"

#include "com/matrix.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"
#include "obj/obj.h"

#include <memory>
#include <vector>

namespace gpu_opengl
{
struct DrawInfo
{
        const opengl::GraphicsProgram* triangles_program;
        const RendererTrianglesMemory* triangles_memory;

        const opengl::GraphicsProgram* points_program;
        const RendererPointsMemory* points_memory;

        const opengl::GraphicsProgram* lines_program;
        const RendererPointsMemory* lines_memory;
};

struct ShadowInfo
{
        const opengl::GraphicsProgram* triangles_program;
        const RendererShadowMemory* triangles_memory;
};

class DrawObject final
{
        class Triangles;
        class Lines;
        class Points;

        mat4 m_model_matrix;

        std::unique_ptr<Triangles> m_triangles;
        std::unique_ptr<Lines> m_lines;
        std::unique_ptr<Points> m_points;

public:
        DrawObject(const Obj<3>& obj, double size, const vec3& position);
        ~DrawObject();

        bool has_shadow() const;
        const mat4& model_matrix() const;
        void draw(const DrawInfo& info) const;
        void shadow(const ShadowInfo& info) const;
};
}

#endif
