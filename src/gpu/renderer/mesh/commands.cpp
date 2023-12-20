/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "commands.h"

#include "object.h"

#include "shaders/descriptors.h"
#include "shaders/program_normals.h"
#include "shaders/program_points.h"
#include "shaders/program_shadow.h"
#include "shaders/program_triangle_lines.h"
#include "shaders/program_triangles.h"

#include <vector>

namespace ns::gpu::renderer
{
void commands_triangles(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const bool transparent,
        const TrianglesProgram& triangles_program,
        const SharedMemory& triangles_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &triangles_shared_memory.descriptor_set(), 0, nullptr);

        push_constant_command(command_buffer, triangles_program.pipeline_layout(), transparent);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        const auto bind_descriptor_set_material = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program.pipeline_layout(),
                        MaterialMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_triangles(
                        command_buffer, triangles_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh,
                        triangles_program.descriptor_set_layout_material(), bind_descriptor_set_material);
        }
}

void commands_shadow(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const ShadowProgram& shadow_program,
        const SharedMemory& shadow_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &shadow_shared_memory.descriptor_set(), 0, nullptr);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_plain_triangles(
                        command_buffer, shadow_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
        }
}

void commands_lines(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const bool transparent,
        const PointsProgram& points_program,
        const SharedMemory& points_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &points_shared_memory.descriptor_set(), 0, nullptr);

        push_constant_command(command_buffer, points_program.pipeline_layout(), transparent);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_lines(
                        command_buffer, points_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
        }
}

void commands_points(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const bool transparent,
        const PointsProgram& points_program,
        const SharedMemory& points_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &points_shared_memory.descriptor_set(), 0, nullptr);

        push_constant_command(command_buffer, points_program.pipeline_layout(), transparent);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_points(
                        command_buffer, points_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
        }
}

void commands_triangle_lines(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const bool transparent,
        const TriangleLinesProgram& triangle_lines_program,
        const SharedMemory& triangle_lines_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_lines_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &triangle_lines_shared_memory.descriptor_set(), 0,
                nullptr);

        push_constant_command(command_buffer, triangle_lines_program.pipeline_layout(), transparent);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_lines_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_plain_triangles(
                        command_buffer, triangle_lines_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
        }
}

void commands_normals(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const VkPipeline pipeline,
        const bool transparent,
        const NormalsProgram& normals_program,
        const SharedMemory& normals_shared_memory)
{
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normals_program.pipeline_layout(),
                SharedMemory::set_number(), 1 /*set count*/, &normals_shared_memory.descriptor_set(), 0, nullptr);

        push_constant_command(command_buffer, normals_program.pipeline_layout(), transparent);

        const auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
        {
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normals_program.pipeline_layout(),
                        MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
        };

        for (const MeshObject* const mesh : meshes)
        {
                mesh->commands_triangle_vertices(
                        command_buffer, normals_program.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
        }
}
}
