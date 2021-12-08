/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "mesh_object.h"

#include "shaders/descriptors.h"
#include "shaders/normals.h"
#include "shaders/points.h"
#include "shaders/triangle_lines.h"
#include "shaders/triangles.h"
#include "shaders/triangles_depth.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace ns::gpu::renderer
{
void commands_triangles(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const TrianglesProgram& triangles_program,
        const CommonMemory& triangles_common_memory);

void commands_depth_triangles(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const TrianglesDepthProgram& triangles_depth_program,
        const CommonMemory& triangles_depth_common_memory);

void commands_lines(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const PointsProgram& points_program,
        const CommonMemory& points_common_memory);

void commands_points(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const PointsProgram& points_program,
        const CommonMemory& points_common_memory);

void commands_triangle_lines(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const TriangleLinesProgram& triangle_lines_program,
        const CommonMemory& triangle_lines_common_memory);

void commands_normals(
        const std::vector<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        VkPipeline pipeline,
        const NormalsProgram& normals_program,
        const CommonMemory& normals_common_memory);
}
