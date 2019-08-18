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

#include "shader_memory.h"

#include "com/matrix.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"
#include "obj/obj.h"

#include <memory>
#include <vector>

namespace gpu_opengl
{
enum class DrawType
{
        Points,
        Lines,
        Triangles
};

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
        opengl::VertexArray m_vertex_array;
        std::unique_ptr<opengl::ArrayBuffer> m_vertex_buffer;
        std::unique_ptr<opengl::StorageBuffer> m_storage_buffer;
        std::vector<opengl::TextureRGBA32F> m_textures;
        unsigned m_vertices_count;

        const mat4 m_model_matrix;
        const DrawType m_draw_type;

        void load_triangles(const Obj<3>& obj);
        void load_points_lines(const Obj<3>& obj);

public:
        DrawObject(const Obj<3>& obj, double size, const vec3& position);

        bool has_shadow() const;
        const mat4& model_matrix() const;
        void draw(const DrawInfo& info) const;
        void shadow(const ShadowInfo& info) const;
};
}
