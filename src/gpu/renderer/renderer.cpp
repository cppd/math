/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "renderer.h"

#include "acceleration_structure.h"
#include "event.h"
#include "functionality.h"
#include "renderer_draw.h"
#include "renderer_object.h"
#include "renderer_view.h"
#include "storage_mesh.h"
#include "storage_volume.h"
#include "viewport_transform.h"

#include "buffers/drawing.h"
#include "buffers/ggx_f1_albedo.h"
#include "buffers/opacity.h"
#include "buffers/transparency.h"
#include "code/code.h"
#include "mesh/object.h"
#include "mesh/renderer.h"
#include "test/ray_tracing/test_ray_tracing.h"
#include "volume/object.h"
#include "volume/renderer.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>
#include <src/vulkan/queue.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>
#include <thread>
#include <variant>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
constexpr bool RAY_TRACING = true;

constexpr VkImageLayout DEPTH_COPY_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

class Impl final : public Renderer, RendererViewEvents, StorageMeshEvents, StorageVolumeEvents
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        numerical::Region<2, int> viewport_;

        const vulkan::Device* const device_;
        const bool ray_tracing_;

        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;
        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;

        const RenderBuffers3D* render_buffers_ = nullptr;
        const vulkan::ImageWithMemory* object_image_ = nullptr;

        DrawingBuffer drawing_buffer_;
        GgxF1Albedo ggx_f1_albedo_;
        TransparencyBuffers transparency_buffers_;
        OpacityBuffers opacity_buffers_;
        std::unique_ptr<vulkan::DepthImageWithMemory> depth_copy_image_;

        MeshRenderer mesh_renderer_;
        VolumeRenderer volume_renderer_;

        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts_{mesh_renderer_.mesh_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_material_layouts_{
                mesh_renderer_.material_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> volume_image_layouts_{
                volume_renderer_.image_layouts()};

        StorageMesh mesh_storage_;
        StorageVolume volume_storage_;
        std::optional<AccelerationStructure> acceleration_structure_;

        RendererObject renderer_object_;
        RendererView renderer_view_;

        RendererDraw renderer_draw_;

        void info(info::Functionality* const functionality) const
        {
                functionality->shadow_zoom = !ray_tracing_;
        }

        void info(info::Description* const description) const
        {
                description->ray_tracing = ray_tracing_;
        }

        void receive(const Info& info) const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                std::visit(
                        [this](const auto& v)
                        {
                                this->info(v);
                        },
                        info);
        }

        void cmd(const ObjectCommand& command)
        {
                renderer_object_.exec(command);
        }

        void cmd(const ViewCommand& command)
        {
                renderer_view_.exec(command);
        }

        void exec(const Command& command) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                std::visit(
                        [this](const auto& v)
                        {
                                cmd(v);
                        },
                        command);
        }

        VkSemaphore draw(
                const VkSemaphore semaphore,
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                const unsigned index) const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(graphics_queue_1.family_index() == graphics_queue_->family_index());
                ASSERT(graphics_queue_2.family_index() == graphics_queue_->family_index());

                const bool shadow_mapping = !ray_tracing_ && renderer_view_.show_shadow();

                return renderer_draw_.draw(
                        semaphore, graphics_queue_1.handle(), graphics_queue_2.handle(), index, shadow_mapping,
                        transparency_buffers_);
        }

        bool empty() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                return !mesh_renderer_.has_meshes() && !volume_renderer_.has_volume();
        }

        void create_buffers(
                RenderBuffers3D* const render_buffers,
                const vulkan::ImageWithMemory* const objects,
                const numerical::Region<2, int>& viewport) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(objects->image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(viewport.x1() <= static_cast<int>(objects->image().extent().width));
                ASSERT(viewport.y1() <= static_cast<int>(objects->image().extent().height));

                render_buffers_ = render_buffers;
                object_image_ = objects;
                viewport_ = viewport;

                const ViewportTransform transform = viewport_transform(viewport_);
                drawing_buffer_.set_viewport(transform.center, transform.factor);

                ASSERT(render_buffers_->framebuffers().size() == 1);

                create_depth_copy_image();
                create_transparency_buffers();
                create_opacity_buffers();

                mesh_renderer_.create_render_buffers(
                        render_buffers_, *object_image_, transparency_buffers_.heads(),
                        transparency_buffers_.heads_size(), transparency_buffers_.counters(),
                        transparency_buffers_.nodes(), opacity_buffers_, viewport_);
                create_mesh_shadow_mapping_buffers();

                volume_renderer_.create_buffers(
                        render_buffers_, viewport_, depth_copy_image_->image_view().handle(),
                        transparency_buffers_.heads(), transparency_buffers_.nodes(), opacity_buffers_);

                create_mesh_command_buffers();
                create_volume_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                volume_renderer_.delete_buffers();
                delete_mesh_shadow_mapping_buffers();
                mesh_renderer_.delete_render_buffers();
                depth_copy_image_.reset();
                transparency_buffers_.delete_buffers();
                opacity_buffers_.delete_buffers();
        }

        void create_depth_copy_image()
        {
                depth_copy_image_ = std::make_unique<vulkan::DepthImageWithMemory>(
                        *device_, std::vector({graphics_queue_->family_index()}),
                        std::vector({render_buffers_->depth_format()}), render_buffers_->sample_count(),
                        render_buffers_->width(), render_buffers_->height(),
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, DEPTH_COPY_IMAGE_LAYOUT,
                        graphics_command_pool_->handle(), graphics_queue_->handle());
        }

        void create_transparency_buffers()
        {
                transparency_buffers_.create_buffers(
                        *device_, *graphics_command_pool_, *graphics_queue_, {graphics_queue_->family_index()},
                        render_buffers_->sample_count(), render_buffers_->width(), render_buffers_->height());

                LOG("Transparency node count: " + to_string_digit_groups(transparency_buffers_.node_count()));

                drawing_buffer_.set_transparency_max_node_count(transparency_buffers_.node_count());
        }

        void create_opacity_buffers()
        {
                opacity_buffers_.create_buffers(
                        *device_, {graphics_queue_->family_index()}, render_buffers_->sample_count(),
                        render_buffers_->width(), render_buffers_->height());
        }

        void delete_mesh_shadow_mapping_buffers()
        {
                if (ray_tracing_)
                {
                        return;
                }

                mesh_renderer_.delete_shadow_mapping_buffers();
        }

        void create_mesh_shadow_mapping_buffers()
        {
                if (ray_tracing_)
                {
                        return;
                }

                ASSERT(render_buffers_);

                delete_mesh_shadow_mapping_buffers();

                mesh_renderer_.create_shadow_mapping_buffers(
                        render_buffers_->framebuffers().size(), {graphics_queue_->family_index()},
                        graphics_command_pool_->handle(), graphics_queue_->handle(), *device_, viewport_.width(),
                        viewport_.height(), renderer_view_.shadow_zoom());

                volume_renderer_.set_shadow_image(
                        mesh_renderer_.shadow_mapping_sampler(), mesh_renderer_.shadow_mapping_image_view());
        }

        void create_mesh_render_command_buffers()
        {
                mesh_renderer_.delete_render_command_buffers();

                mesh_renderer_.create_render_command_buffers(
                        mesh_storage_.visible_objects(), graphics_command_pool_->handle(),
                        renderer_view_.clip_plane().has_value() && renderer_view_.show_clip_plane_lines(),
                        renderer_view_.show_normals(),
                        [this](const VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_.commands_init(command_buffer);
                        },
                        [this](const VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_.commands_read(command_buffer);
                        });
        }

        void create_mesh_shadow_mapping_command_buffers()
        {
                if (ray_tracing_)
                {
                        return;
                }

                mesh_renderer_.delete_shadow_mapping_command_buffers();

                mesh_renderer_.create_shadow_mapping_command_buffers(
                        mesh_storage_.visible_objects(), graphics_command_pool_->handle());
        }

        void create_mesh_command_buffers()
        {
                create_mesh_render_command_buffers();
                create_mesh_shadow_mapping_command_buffers();
        }

        void create_volume_command_buffers()
        {
                volume_renderer_.delete_command_buffers();

                if (volume_storage_.visible_objects().size() != 1)
                {
                        volume_renderer_.create_command_buffers(graphics_command_pool_->handle());
                        return;
                }

                const auto copy_depth = [this](const VkCommandBuffer command_buffer)
                {
                        ASSERT(render_buffers_);
                        ASSERT(render_buffers_->framebuffers().size() == 1);
                        constexpr int INDEX = 0;

                        render_buffers_->commands_depth_copy(
                                command_buffer, depth_copy_image_->image().handle(), DEPTH_COPY_IMAGE_LAYOUT, viewport_,
                                INDEX);
                };

                volume_renderer_.create_command_buffers(
                        volume_storage_.visible_objects().front(), graphics_command_pool_->handle(), copy_depth);
        }

        void set_volume_matrix()
        {
                if (ray_tracing_)
                {
                        for (VolumeObject* const visible_volume : volume_storage_.visible_objects())
                        {
                                visible_volume->set_matrix_and_clip_plane(
                                        renderer_view_.vp_matrix(), renderer_view_.clip_plane());
                        }
                }
                else
                {
                        for (VolumeObject* const visible_volume : volume_storage_.visible_objects())
                        {
                                visible_volume->set_matrix_and_clip_plane(
                                        renderer_view_.vp_matrix(), renderer_view_.clip_plane(),
                                        renderer_view_.world_to_shadow_matrix());
                        }
                }
        }

        void acceleration_structure_create()
        {
                if (!ray_tracing_)
                {
                        return;
                }
                ASSERT(acceleration_structure_);
                acceleration_structure_->create(
                        *device_, *compute_command_pool_, *compute_queue_, mesh_storage_.visible_objects());
                mesh_renderer_.set_acceleration_structure(acceleration_structure_->handle());
                volume_renderer_.set_acceleration_structure(acceleration_structure_->handle());
                create_mesh_render_command_buffers();
                create_volume_command_buffers();
        }

        void acceleration_structure_update_matrices() const
        {
                if (!ray_tracing_)
                {
                        return;
                }
                ASSERT(acceleration_structure_);
                acceleration_structure_->update_matrices(
                        device_->handle(), *compute_command_pool_, *compute_queue_, mesh_storage_.visible_objects());
        }

        // StorageMeshEvents

        std::unique_ptr<MeshObject> mesh_create() override
        {
                return create_mesh_object(
                        device_, ray_tracing_, {graphics_queue_->family_index()}, compute_command_pool_, compute_queue_,
                        transfer_command_pool_, transfer_queue_, mesh_layouts_, mesh_material_layouts_,
                        mesh_renderer_.texture_sampler());
        }

        void mesh_visibility_changed() override
        {
                create_mesh_command_buffers();
                acceleration_structure_create();
        }

        void mesh_visible_changed(const MeshObject::UpdateChanges& update_changes) override
        {
                if (update_changes.mesh || update_changes.transparency)
                {
                        create_mesh_command_buffers();
                }

                if (update_changes.mesh)
                {
                        acceleration_structure_create();
                }
                else if (update_changes.matrix)
                {
                        acceleration_structure_update_matrices();
                }
        }

        // StorageVolumeEvents

        std::unique_ptr<VolumeObject> volume_create() override
        {
                return create_volume_object(
                        ray_tracing_, device_, {graphics_queue_->family_index()}, transfer_command_pool_,
                        transfer_queue_, volume_image_layouts_, volume_renderer_.image_sampler(),
                        volume_renderer_.transfer_function_sampler());
        }

        void volume_visibility_changed() override
        {
                create_volume_command_buffers();
                set_volume_matrix();
        }

        void volume_visible_changed(const VolumeObject::UpdateChanges& update_changes) override
        {
                if (update_changes.image)
                {
                        create_volume_command_buffers();
                }
        }

        // RendererViewEvents

        void view_show_normals_changed() override
        {
                create_mesh_render_command_buffers();
        }

        void view_shadow_zoom_changed() override
        {
                create_mesh_shadow_mapping_buffers();
                create_mesh_command_buffers();
                create_volume_command_buffers();
        }

        void view_matrices_changed() override
        {
                set_volume_matrix();
                if (ray_tracing_)
                {
                        volume_renderer_.set_matrix(renderer_view_.vp_matrix());
                }
                else
                {
                        mesh_renderer_.set_shadow_matrices(
                                renderer_view_.shadow_vp_matrix(), renderer_view_.world_to_shadow_matrix());
                        volume_renderer_.set_matrix(
                                renderer_view_.vp_matrix(), renderer_view_.world_to_shadow_matrix());
                }
        }

        void view_clip_plane_changed(const bool visibility_changed) override
        {
                if (visibility_changed && renderer_view_.show_clip_plane_lines())
                {
                        create_mesh_render_command_buffers();
                }
                if (const auto& clip_plane = renderer_view_.clip_plane())
                {
                        for (VolumeObject* const visible_volume : volume_storage_.visible_objects())
                        {
                                visible_volume->set_clip_plane(*clip_plane);
                        }
                }
        }

        void view_show_clip_plane_lines_changed() override
        {
                if (renderer_view_.clip_plane())
                {
                        create_mesh_render_command_buffers();
                }
        }

public:
        Impl(const vulkan::Device* const device,
             const Code& code,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue,
             const bool sample_shading,
             const bool sampler_anisotropy)
                : device_(device),
                  ray_tracing_(code.ray_tracing()),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  drawing_buffer_(*device_, {graphics_queue_->family_index()}),
                  ggx_f1_albedo_(
                          *device_,
                          {graphics_queue_->family_index()},
                          *transfer_command_pool_,
                          *transfer_queue_),
                  transparency_buffers_(ray_tracing_, *device_, {graphics_queue_->family_index()}),
                  opacity_buffers_(ray_tracing_),
                  mesh_renderer_(
                          device_,
                          code,
                          sample_shading,
                          sampler_anisotropy,
                          drawing_buffer_.buffer(),
                          {graphics_queue_->family_index()},
                          ggx_f1_albedo_),
                  volume_renderer_(
                          device_,
                          code,
                          sample_shading,
                          {graphics_queue_->family_index()},
                          drawing_buffer_.buffer(),
                          ggx_f1_albedo_),
                  mesh_storage_(this),
                  volume_storage_(this),
                  renderer_object_(&mesh_storage_, &volume_storage_),
                  renderer_view_(!ray_tracing_, &drawing_buffer_, this),
                  renderer_draw_(
                          device_->handle(),
                          transparency_buffers_.buffer_size(),
                          &mesh_renderer_,
                          &volume_renderer_)
        {
                if (ray_tracing_)
                {
                        acceleration_structure_.emplace(
                                *device_, *compute_command_pool_, *compute_queue_,
                                std::vector({graphics_queue_->family_index()}));
                        mesh_renderer_.set_acceleration_structure(acceleration_structure_->handle());
                        volume_renderer_.set_acceleration_structure(acceleration_structure_->handle());
                }
        }

        ~Impl() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                device_->wait_idle_noexcept("renderer destructor");
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

vulkan::physical_device::DeviceFunctionality Renderer::device_functionality()
{
        vulkan::physical_device::DeviceFunctionality res = renderer::device_functionality();
        if (RAY_TRACING)
        {
                res.merge(renderer::device_ray_tracing_functionality());
        }
        return res;
}

std::unique_ptr<Renderer> create_renderer(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const bool sample_shading,
        const bool sampler_anisotropy)
{
        const bool ray_tracing = ray_tracing_supported(*device);

        if (ray_tracing)
        {
                test::test_ray_tracing(*device, *compute_queue);
        }

        const Code code(ray_tracing);

        return std::make_unique<Impl>(
                device, code, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                compute_command_pool, compute_queue, sample_shading, sampler_anisotropy);
}
}
