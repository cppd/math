/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "object.h"

#include "shaders/descriptors.h"
#include "shaders/program_normals.h"
#include "shaders/program_points.h"
#include "shaders/program_shadow.h"
#include "shaders/program_triangle_lines.h"
#include "shaders/program_triangles.h"

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
void commands_triangles(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        bool transparent,
        const TrianglesProgram& triangles_program,
        const SharedMemory& triangles_shared_memory);

void commands_shadow(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const ShadowProgram& shadow_program,
        const SharedMemory& shadow_shared_memory);

void commands_lines(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        bool transparent,
        const PointsProgram& points_program,
        const SharedMemory& points_shared_memory);

void commands_points(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        bool transparent,
        const PointsProgram& points_program,
        const SharedMemory& points_shared_memory);

void commands_triangle_lines(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        bool transparent,
        const TriangleLinesProgram& triangle_lines_program,
        const SharedMemory& triangle_lines_shared_memory);

void commands_normals(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        bool transparent,
        const NormalsProgram& normals_program,
        const SharedMemory& normals_shared_memory);
}
