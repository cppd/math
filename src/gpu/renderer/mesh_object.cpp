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

#include "mesh_object.h"

#include "mesh_object_load.h"
#include "shading_parameters.h"

#include "shaders/buffers.h"
#include "shaders/descriptors.h"
#include "shaders/triangles.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/merge.h>
#include <src/model/mesh_utility.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>

#include <algorithm>
#include <array>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
std::vector<TrianglesMaterialMemory::MaterialInfo> materials_info(
        const mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        const std::vector<MaterialBuffer>& material_buffers)
{
        // one more texture and material for specifying but not using
        ASSERT(textures.size() == mesh.images.size() + 1);
        ASSERT(material_buffers.size() == mesh.materials.size() + 1);

        VkImageView no_texture = textures.back().image_view();

        std::vector<TrianglesMaterialMemory::MaterialInfo> materials;
        materials.reserve(mesh.materials.size() + 1);

        for (std::size_t i = 0; i < mesh.materials.size(); ++i)
        {
                const typename mesh::Mesh<3>::Material& mesh_material = mesh.materials[i];

                ASSERT(mesh_material.image < static_cast<int>(textures.size()) - 1);

                TrianglesMaterialMemory::MaterialInfo& m = materials.emplace_back();
                m.buffer = material_buffers[i].buffer();
                m.buffer_size = material_buffers[i].buffer().size();
                m.texture = (mesh_material.image >= 0) ? textures[mesh_material.image].image_view() : no_texture;
        }

        TrianglesMaterialMemory::MaterialInfo& m = materials.emplace_back();
        m.buffer = material_buffers.back().buffer();
        m.buffer_size = material_buffers.back().buffer().size();
        m.texture = no_texture;

        return materials;
}

class Impl final : public MeshObject
{
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const std::vector<std::uint32_t> family_indices_;

        MeshBuffer mesh_buffer_;
        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> mesh_descriptor_sets_;
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts_;

        struct MaterialVertices
        {
                int offset;
                int count;
        };
        std::vector<MaterialVertices> material_vertices_;

        std::unique_ptr<vulkan::BufferWithMemory> faces_vertex_buffer_;
        std::unique_ptr<vulkan::BufferWithMemory> faces_index_buffer_;
        unsigned faces_vertex_count_ = 0;
        unsigned faces_index_count_ = 0;

        std::vector<vulkan::ImageWithMemory> textures_;
        std::vector<MaterialBuffer> material_buffers_;
        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> material_descriptor_sets_;
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts_;
        VkSampler texture_sampler_;

        std::unique_ptr<vulkan::BufferWithMemory> lines_vertex_buffer_;
        unsigned lines_vertex_count_ = 0;

        std::unique_ptr<vulkan::BufferWithMemory> points_vertex_buffer_;
        unsigned points_vertex_count_ = 0;

        bool transparent_ = false;

        std::optional<int> version_;

        void buffer_set_lighting(float ambient, float metalness, float roughness) const
        {
                std::tie(ambient, metalness, roughness) = clean_shading_parameters(ambient, metalness, roughness);

                mesh_buffer_.set_lighting(ambient, metalness, roughness);
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

        void buffer_set_coordinates(const Matrix4d& model_matrix)
        {
                mesh_buffer_.set_coordinates(model_matrix, model_matrix.top_left<3, 3>().inverse().transpose());
        }

        void create_mesh_descriptor_sets()
        {
                mesh_descriptor_sets_.clear();
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : mesh_layouts_)
                {
                        vulkan::Descriptors sets = MeshMemory::create(
                                *device_, layout.descriptor_set_layout, layout.descriptor_set_layout_bindings,
                                {&mesh_buffer_.buffer()});
                        ASSERT(sets.descriptor_set_count() == 1);
                        mesh_descriptor_sets_.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        VkDescriptorSet find_mesh_descriptor_set(VkDescriptorSetLayout mesh_descriptor_set_layout) const
        {
                auto iter = mesh_descriptor_sets_.find(mesh_descriptor_set_layout);
                if (iter == mesh_descriptor_sets_.cend())
                {
                        error("Failed to find mesh descriptor sets for mesh descriptor set layout");
                }
                ASSERT(iter->second.descriptor_set_count() == 1);
                return iter->second.descriptor_set(0);
        }

        void create_material_descriptor_sets(const std::vector<TrianglesMaterialMemory::MaterialInfo>& material_info)
        {
                material_descriptor_sets_.clear();
                if (material_info.empty())
                {
                        return;
                }
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : material_layouts_)
                {
                        vulkan::Descriptors sets = TrianglesMaterialMemory::create(
                                *device_, texture_sampler_, layout.descriptor_set_layout,
                                layout.descriptor_set_layout_bindings, material_info);
                        ASSERT(sets.descriptor_set_count() == material_info.size());
                        material_descriptor_sets_.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        const vulkan::Descriptors& find_material_descriptor_sets(
                VkDescriptorSetLayout material_descriptor_set_layout) const
        {
                auto iter = material_descriptor_sets_.find(material_descriptor_set_layout);
                if (iter == material_descriptor_sets_.cend())
                {
                        error("Failed to find material descriptor sets for material descriptor set layout");
                }

                ASSERT(iter->second.descriptor_set_count() == material_vertices_.size());

                return iter->second;
        }

        //

        void load_mesh_textures_and_materials(const mesh::Mesh<3>& mesh)
        {
                if (mesh.facets.empty())
                {
                        textures_.clear();
                        material_buffers_.clear();
                        create_material_descriptor_sets({});
                        return;
                }

                textures_ = load_textures(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);

                material_buffers_ =
                        load_materials(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);

                create_material_descriptor_sets(materials_info(mesh, textures_, material_buffers_));
        }

        void load_mesh_vertices(const mesh::Mesh<3>& mesh)
        {
                {
                        std::vector<int> sorted_face_indices;
                        std::vector<int> material_face_offset;
                        std::vector<int> material_face_count;

                        mesh::sort_facets_by_material(
                                mesh, &sorted_face_indices, &material_face_offset, &material_face_count);

                        ASSERT(material_face_offset.size() == material_face_count.size());
                        ASSERT(std::all_of(
                                material_descriptor_sets_.cbegin(), material_descriptor_sets_.cend(),
                                [&](const auto& v)
                                {
                                        return material_face_offset.size() == v.second.descriptor_set_count();
                                }));

                        material_vertices_.resize(material_face_count.size());
                        for (unsigned i = 0; i < material_face_count.size(); ++i)
                        {
                                material_vertices_[i].offset = 3 * material_face_offset[i];
                                material_vertices_[i].count = 3 * material_face_count[i];
                        }

                        load_vertices(
                                *device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh,
                                sorted_face_indices, &faces_vertex_buffer_, &faces_index_buffer_, &faces_vertex_count_,
                                &faces_index_count_);

                        ASSERT(faces_index_count_ == 3 * mesh.facets.size());
                }

                lines_vertex_buffer_ =
                        load_line_vertices(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);
                lines_vertex_count_ = 2 * mesh.lines.size();

                points_vertex_buffer_ =
                        load_point_vertices(*device_, *transfer_command_pool_, *transfer_queue_, family_indices_, mesh);
                points_vertex_count_ = mesh.points.size();
        }

        //

        bool transparent() const override
        {
                return transparent_;
        }

        void commands_triangles(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set,
                VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const vulkan::Descriptors& descriptor_sets =
                        find_material_descriptor_sets(material_descriptor_set_layout);

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, faces_index_buffer_->buffer(), 0, VERTEX_INDEX_TYPE);

                for (unsigned i = 0; i < material_vertices_.size(); ++i)
                {
                        if (material_vertices_[i].count <= 0)
                        {
                                continue;
                        }

                        bind_material_descriptor_set(descriptor_sets.descriptor_set(i));

                        vkCmdDrawIndexed(
                                command_buffer, material_vertices_[i].count, 1, material_vertices_[i].offset, 0, 0);
                }
        }

        void commands_plain_triangles(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());
                vkCmdBindIndexBuffer(command_buffer, faces_index_buffer_->buffer(), 0, VERTEX_INDEX_TYPE);

                vkCmdDrawIndexed(command_buffer, faces_index_count_, 1, 0, 0, 0);
        }

        void commands_triangle_vertices(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (faces_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{faces_vertex_buffer_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, faces_vertex_count_, 1, 0, 0);
        }

        void commands_lines(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (lines_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{lines_vertex_buffer_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, lines_vertex_count_, 1, 0, 0);
        }

        void commands_points(
                VkCommandBuffer command_buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const override
        {
                if (points_vertex_count_ == 0)
                {
                        return;
                }

                bind_mesh_descriptor_set(find_mesh_descriptor_set(mesh_descriptor_set_layout));

                const std::array<VkBuffer, 1> buffers{points_vertex_buffer_->buffer()};
                const std::array<VkDeviceSize, 1> offsets{0};

                vkCmdBindVertexBuffers(command_buffer, 0, buffers.size(), buffers.data(), offsets.data());

                vkCmdDraw(command_buffer, points_vertex_count_, 1, 0, 0);
        }

        UpdateChanges update(const mesh::Reading<3>& mesh_object) override
        {
                const mesh::Updates updates = mesh_object.updates(&version_);
                if (updates.none())
                {
                        return {};
                }

                UpdateChanges update_changes;

                ASSERT(!mesh_object.mesh().facets.empty() || !mesh_object.mesh().lines.empty()
                       || !mesh_object.mesh().points.empty());

                static_assert(mesh::Updates().size() == 8);

                static constexpr mesh::Updates LIGHTING_UPDATES(
                        (1ull << mesh::UPDATE_AMBIENT) | (1ull << mesh::UPDATE_METALNESS)
                        | (1ull << mesh::UPDATE_ROUGHNESS));

                if (updates[mesh::UPDATE_MATRIX])
                {
                        buffer_set_coordinates(mesh_object.matrix());
                }

                if (updates[mesh::UPDATE_ALPHA])
                {
                        buffer_set_alpha(mesh_object.alpha());

                        bool transparent = mesh_object.alpha() < 1;
                        if (transparent_ != transparent)
                        {
                                transparent_ = transparent;
                                update_changes.transparency = true;
                        }
                }

                if (updates[mesh::UPDATE_COLOR])
                {
                        buffer_set_color(mesh_object.color());
                }

                if ((updates & LIGHTING_UPDATES).any())
                {
                        buffer_set_lighting(mesh_object.ambient(), mesh_object.metalness(), mesh_object.roughness());
                }

                if (updates[mesh::UPDATE_MESH])
                {
                        const mesh::Mesh<3>& mesh = mesh_object.mesh();

                        load_mesh_textures_and_materials(mesh);
                        load_mesh_vertices(mesh);

                        update_changes.command_buffers = true;
                }

                if (updates[mesh::UPDATE_VISIBILITY])
                {
                        update_changes.visibility = true;
                }

                return update_changes;
        }

public:
        Impl(const vulkan::Device* const device,
             const std::vector<std::uint32_t>& graphics_family_indices,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
             const VkSampler texture_sampler)
                : device_(device),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  family_indices_(sort_and_unique(
                          merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue->family_index()))),
                  mesh_buffer_(*device, graphics_family_indices),
                  mesh_layouts_(std::move(mesh_layouts)),
                  material_layouts_(std::move(material_layouts)),
                  texture_sampler_(texture_sampler)
        {
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());

                create_mesh_descriptor_sets();
        }
};
}

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device* const device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
        const VkSampler texture_sampler)
{
        return std::make_unique<Impl>(
                device, graphics_family_indices, transfer_command_pool, transfer_queue, std::move(mesh_layouts),
                std::move(material_layouts), texture_sampler);
}
}
