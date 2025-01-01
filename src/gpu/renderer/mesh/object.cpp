/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "object.h"

#include "load.h"
#include "memory.h"

#include "buffers/material.h"
#include "buffers/mesh.h"
#include "shaders/descriptors.h"

#include <src/color/color.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/merge.h>
#include <src/gpu/renderer/shading_parameters.h>
#include <src/model/mesh.h>
#include <src/model/mesh_object.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
struct MaterialVertices final
{
        int offset;
        int count;
};

class Impl final : public MeshObject
{
        const vulkan::Device* const device_;
        const bool ray_tracing_;

        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const std::vector<std::uint32_t> family_indices_;
        const std::vector<std::uint32_t> acceleration_structure_family_indices_;

        const MeshBuffer mesh_buffer_;
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts_;
        const std::unordered_map<VkDescriptorSetLayout, MeshMemory> mesh_memory_;

        std::vector<MaterialVertices> material_vertices_;

        std::unique_ptr<vulkan::BufferWithMemory> faces_vertex_buffer_;
        std::unique_ptr<vulkan::BufferWithMemory> faces_index_buffer_;
        unsigned faces_vertex_count_ = 0;
        unsigned faces_index_count_ = 0;

        const VkSampler texture_sampler_;
        std::vector<vulkan::ImageWithMemory> textures_;

        const std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts_;
        std::vector<MaterialBuffer> material_buffers_;
        std::unordered_map<VkDescriptorSetLayout, MaterialMemory> material_memory_;

        std::unique_ptr<vulkan::BufferWithMemory> lines_vertex_buffer_;
        unsigned lines_vertex_count_ = 0;

        std::unique_ptr<vulkan::BufferWithMemory> points_vertex_buffer_;
        unsigned points_vertex_count_ = 0;

        std::unique_ptr<vulkan::BottomLevelAccelerationStructure> acceleration_structure_;
        VkTransformMatrixKHR transform_matrix_;

        bool transparent_ = false;

        std::optional<int> version_;

        void buffer_set_lighting(const float ambient, const float metalness, const float roughness) const
        {
                mesh_buffer_.set_lighting(
                        clean_ambient(ambient), clean_metalness(metalness), clean_roughness(roughness));
        }

        void buffer_set_color(const color::Color& color)
        {
                mesh_buffer_.set_color(color.rgb32().clamp(0, 1));
        }

        void buffer_set_alpha(float alpha)
        {
                alpha = std::clamp(alpha, 0.0f, 1.0f);
                mesh_buffer_.set_alpha(alpha);
        }

        void buffer_set_coordinates(const numerical::Matrix4d& model_matrix)
        {
                mesh_buffer_.set_coordinates(model_matrix, model_matrix.top_left<3, 3>().inversed().transposed());
        }

        void set_transform_matrix(const numerical::Matrix4d& model_matrix)
        {
                if (ray_tracing_)
                {
                        for (std::size_t i = 0; i < 3; ++i)
                        {
                                for (std::size_t j = 0; j < 4; ++j)
                                {
                                        transform_matrix_.matrix[i][j] = model_matrix[i, j];
                                }
                        }
                }
        }

        VkDescriptorSet find_mesh_descriptor_set(const VkDescriptorSetLayout mesh_descriptor_set_layout) const
        {
                const auto iter = mesh_memory_.find(mesh_descriptor_set_layout);
                if (iter != mesh_memory_.cend())
                {
                        return iter->second.descriptor_set();
                }
                error("Failed to find mesh memory for mesh descriptor set layout");
        }

        const MaterialMemory& find_material_memory(const VkDescriptorSetLayout material_descriptor_set_layout) const
        {
                const auto iter = material_memory_.find(material_descriptor_set_layout);
                if (iter != material_memory_.cend())
                {
                        return iter->second;
                }
                error("Failed to find material memory for material descriptor set layout");
        }

        //

        void load_mesh_textures_and_materials(const model::mesh::Mesh<3>& mesh)
        {
                textures_.clear();
                material_buffers_.clear();
                material_memory_.clear();

                if (mesh.facets.empty())
                {
                        return;
                }

                textures_ = load_textures(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);

                material_buffers_ =
                        load_materials(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);

                material_memory_ = create_material_memory(
                        device_->handle(), texture_sampler_, material_layouts_, mesh, textures_, material_buffers_);
        }

        void load_mesh_geometry_vertices(const model::mesh::Mesh<3>& mesh)
        {
                const model::mesh::SortedFacets facets = model::mesh::sort_facets_by_material(mesh);

                ASSERT(facets.offset.size() == facets.count.size());
                ASSERT(std::ranges::all_of(
                        material_memory_,
                        [&](const auto& v)
                        {
                                return facets.offset.size() == v.second.descriptor_set_count();
                        }));

                material_vertices_.resize(facets.count.size());
                for (std::size_t i = 0; i < facets.count.size(); ++i)
                {
                        material_vertices_[i].offset = 3 * facets.offset[i];
                        material_vertices_[i].count = 3 * facets.count[i];
                }

                BufferMesh buffer_mesh;

                load_vertices(
                        *device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh, facets.indices,
                        &faces_vertex_buffer_, &faces_index_buffer_, &buffer_mesh);

                faces_vertex_count_ = buffer_mesh.vertices.size();
                faces_index_count_ = buffer_mesh.indices.size();

                ASSERT(faces_index_count_ == 3 * mesh.facets.size());

                if (ray_tracing_)
                {
                        acceleration_structure_ = load_acceleration_structure(
                                *device_, *compute_command_pool_, *compute_queue_,
                                acceleration_structure_family_indices_, buffer_mesh);
                }
        }

        void load_mesh_geometry(const model::mesh::Mesh<3>& mesh)
        {
                faces_vertex_buffer_.reset();
                faces_index_buffer_.reset();
                lines_vertex_buffer_.reset();
                points_vertex_buffer_.reset();
                acceleration_structure_.reset();

                load_mesh_geometry_vertices(mesh);

                lines_vertex_buffer_ =
                        load_line_vertices(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);
                lines_vertex_count_ = 2 * mesh.lines.size();

                points_vertex_buffer_ =
                        load_point_vertices(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);
                points_vertex_count_ = mesh.points.size();
        }

        //

        [[nodiscard]] bool transparent() const override
        {
                return transparent_;
        }

        void commands_triangles(
                const VkCommandBuffer command_buffer,
                const VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set,
                const VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const MaterialMemory& material_memory = find_material_memory(material_descriptor_set_layout);

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, faces_index_buffer_->buffer().handle(), 0, VERTEX_INDEX_TYPE);

                for (std::size_t i = 0; i < material_vertices_.size(); ++i)
                {
                        if (material_vertices_[i].count <= 0)
                        {
                                continue;
                        }

                        bind_material_descriptor_set(material_memory.descriptor_set(i));

                        vkCmdDrawIndexed(
                                command_buffer, material_vertices_[i].count, 1, material_vertices_[i].offset, 0, 0);
                }
        }

        void commands_plain_triangles(
                const VkCommandBuffer command_buffer,
                const VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, faces_index_buffer_->buffer().handle(), 0, VERTEX_INDEX_TYPE);

                vkCmdDrawIndexed(command_buffer, faces_index_count_, 1, 0, 0, 0);
        }

        void commands_triangle_vertices(
                const VkCommandBuffer command_buffer,
                const VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, faces_vertex_count_, 1, 0, 0);
        }

        void commands_lines(
                const VkCommandBuffer command_buffer,
                const VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (lines_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{lines_vertex_buffer_->buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, lines_vertex_count_, 1, 0, 0);
        }

        void commands_points(
                const VkCommandBuffer command_buffer,
                const VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (points_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{points_vertex_buffer_->buffer().handle()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, points_vertex_count_, 1, 0, 0);
        }

        UpdateChanges update(const model::mesh::Reading<3>& mesh_object) override
        {
                const model::mesh::Updates updates = mesh_object.updates(&version_);
                if (updates.none())
                {
                        return {};
                }

                UpdateChanges update_changes;

                ASSERT(!mesh_object.mesh().facets.empty() || !mesh_object.mesh().lines.empty()
                       || !mesh_object.mesh().points.empty());

                static_assert(model::mesh::Updates().size() == 8);

                static constexpr model::mesh::Updates LIGHTING_UPDATES(
                        (1ull << model::mesh::UPDATE_AMBIENT) | (1ull << model::mesh::UPDATE_METALNESS)
                        | (1ull << model::mesh::UPDATE_ROUGHNESS));

                if (updates[model::mesh::UPDATE_MATRIX])
                {
                        buffer_set_coordinates(mesh_object.matrix());
                        set_transform_matrix(mesh_object.matrix());

                        update_changes.matrix = true;
                }

                if (updates[model::mesh::UPDATE_ALPHA])
                {
                        buffer_set_alpha(mesh_object.alpha());

                        const bool transparent = mesh_object.alpha() < 1;
                        if (transparent_ != transparent)
                        {
                                transparent_ = transparent;
                                update_changes.transparency = true;
                        }
                }

                if (updates[model::mesh::UPDATE_COLOR])
                {
                        buffer_set_color(mesh_object.color());
                }

                if ((updates & LIGHTING_UPDATES).any())
                {
                        buffer_set_lighting(mesh_object.ambient(), mesh_object.metalness(), mesh_object.roughness());
                }

                if (updates[model::mesh::UPDATE_MESH])
                {
                        const model::mesh::Mesh<3>& mesh = mesh_object.mesh();

                        load_mesh_textures_and_materials(mesh);
                        load_mesh_geometry(mesh);

                        update_changes.mesh = true;
                }

                return update_changes;
        }

        [[nodiscard]] std::optional<VkDeviceAddress> acceleration_structure_device_address() const override
        {
                ASSERT(ray_tracing_);
                if (acceleration_structure_)
                {
                        return acceleration_structure_->device_address();
                }
                return {};
        }

        [[nodiscard]] const VkTransformMatrixKHR& acceleration_structure_matrix() const override
        {
                ASSERT(ray_tracing_);
                return transform_matrix_;
        }

public:
        Impl(const vulkan::Device* const device,
             const bool ray_tracing,
             const std::vector<std::uint32_t>& graphics_family_indices,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
             const VkSampler texture_sampler)
                : device_(device),
                  ray_tracing_(ray_tracing),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  family_indices_(sort_and_unique(
                          merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue->family_index()))),
                  acceleration_structure_family_indices_(graphics_family_indices),
                  mesh_buffer_(*device, graphics_family_indices),
                  mesh_layouts_(std::move(mesh_layouts)),
                  mesh_memory_(create_mesh_memory(device_->handle(), mesh_layouts_, mesh_buffer_.buffer())),
                  texture_sampler_(texture_sampler),
                  material_layouts_(std::move(material_layouts))
        {
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());
        }
};
}

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device* const device,
        const bool ray_tracing,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
        const VkSampler texture_sampler)
{
        return std::make_unique<Impl>(
                device, ray_tracing, graphics_family_indices, compute_command_pool, compute_queue,
                transfer_command_pool, transfer_queue, std::move(mesh_layouts), std::move(material_layouts),
                texture_sampler);
}
}
