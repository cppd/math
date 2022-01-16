/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "buffer_commands.h"
#include "renderer_draw.h"
#include "renderer_objects.h"
#include "renderer_process.h"
#include "storage_mesh.h"
#include "storage_volume.h"
#include "viewport_transform.h"

#include "buffers/ggx_f1_albedo.h"
#include "buffers/shader.h"
#include "buffers/transparency.h"
#include "mesh/object.h"
#include "mesh/renderer.h"
#include "volume/object.h"
#include "volume/renderer.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/error.h>
#include <src/vulkan/query.h>
#include <src/vulkan/queue.h>

#include <memory>
#include <optional>
#include <thread>

namespace ns::gpu::renderer
{
namespace
{
constexpr VkImageLayout DEPTH_COPY_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
constexpr std::uint32_t OBJECTS_CLEAR_VALUE = 0;
constexpr std::uint32_t TRANSPARENCY_NODE_BUFFER_MAX_SIZE = (1ull << 30);

class Impl final : public Renderer, RendererProcessEvents
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        Region<2, int> viewport_;

        const vulkan::DeviceInstance* const instance_;
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const RenderBuffers3D* render_buffers_ = nullptr;
        const vulkan::ImageWithMemory* object_image_ = nullptr;

        ShaderBuffers shader_buffers_;
        GgxF1Albedo ggx_f1_albedo_;
        std::unique_ptr<vulkan::DepthImageWithMemory> depth_copy_image_;

        MeshRenderer mesh_renderer_;
        VolumeRenderer volume_renderer_;

        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts_{mesh_renderer_.mesh_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_material_layouts_{
                mesh_renderer_.material_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> volume_image_layouts_{
                volume_renderer_.image_layouts()};

        std::optional<vulkan::handle::CommandBuffers> clear_command_buffers_;
        std::optional<TransparencyBuffers> transparency_buffers_;

        RendererObjects renderer_objects_;
        RendererProcess renderer_process_;
        RendererDraw renderer_draw_;

        void command(const ObjectCommand& object_command)
        {
                renderer_objects_.command(object_command);
        }

        void command(const ViewCommand& view_command)
        {
                renderer_process_.exec(view_command);
        }

        void exec(Command&& renderer_command) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                std::visit(visitor, renderer_command);
        }

        VkSemaphore draw(
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                const unsigned index) const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(graphics_queue_1.family_index() == graphics_queue_->family_index());
                ASSERT(graphics_queue_2.family_index() == graphics_queue_->family_index());
                ASSERT(clear_command_buffers_);
                ASSERT(transparency_buffers_);

                return renderer_draw_.draw(
                        graphics_queue_1, graphics_queue_2, index, renderer_process_.show_shadow(),
                        *clear_command_buffers_, *transparency_buffers_);
        }

        bool empty() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                return !mesh_renderer_.render_command_buffer_all(0).has_value() && !volume_renderer_.has_volume();
        }

        void create_buffers(
                RenderBuffers3D* const render_buffers,
                const vulkan::ImageWithMemory* const objects,
                const Region<2, int>& viewport) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(objects->image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(viewport.x1() <= static_cast<int>(objects->image().extent().width));
                ASSERT(viewport.y1() <= static_cast<int>(objects->image().extent().height));

                render_buffers_ = render_buffers;
                object_image_ = objects;
                viewport_ = viewport;

                const ViewportTransform transform = viewport_transform(viewport_);
                shader_buffers_.set_viewport(transform.center, transform.factor);

                ASSERT(render_buffers_->framebuffers().size() == render_buffers_->framebuffers_clear().size());
                ASSERT(render_buffers_->framebuffers().size() == 1);

                create_depth_image();
                create_transparency_buffers();

                mesh_renderer_.create_render_buffers(
                        render_buffers_, *object_image_, transparency_buffers_->heads(),
                        transparency_buffers_->heads_size(), transparency_buffers_->counters(),
                        transparency_buffers_->nodes(), viewport_);
                create_mesh_depth_buffers();

                volume_renderer_.create_buffers(
                        render_buffers_, *graphics_command_pool_, viewport_, depth_copy_image_->image_view(),
                        transparency_buffers_->heads(), transparency_buffers_->nodes());

                create_mesh_command_buffers();
                create_volume_command_buffers();
                create_clear_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                clear_command_buffers_.reset();
                volume_renderer_.delete_buffers();
                delete_mesh_depth_buffers();
                mesh_renderer_.delete_render_buffers();
                depth_copy_image_.reset();
                delete_transparency_buffers();
        }

        void create_depth_image()
        {
                depth_copy_image_ = std::make_unique<vulkan::DepthImageWithMemory>(
                        *device_, std::vector<std::uint32_t>({graphics_queue_->family_index()}),
                        std::vector<VkFormat>({render_buffers_->depth_format()}), render_buffers_->sample_count(),
                        render_buffers_->width(), render_buffers_->height(),
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, DEPTH_COPY_IMAGE_LAYOUT,
                        *graphics_command_pool_, *graphics_queue_);
        }

        void create_transparency_buffers()
        {
                transparency_buffers_.emplace(
                        *device_, *graphics_command_pool_, *graphics_queue_,
                        std::vector<std::uint32_t>({graphics_queue_->family_index()}), render_buffers_->sample_count(),
                        render_buffers_->width(), render_buffers_->height(), TRANSPARENCY_NODE_BUFFER_MAX_SIZE);

                LOG("Transparency node count: " + to_string_digit_groups(transparency_buffers_->node_count()));

                shader_buffers_.set_transparency_max_node_count(transparency_buffers_->node_count());
        }

        void delete_transparency_buffers()
        {
                transparency_buffers_.reset();
        }

        void delete_mesh_depth_buffers()
        {
                mesh_renderer_.delete_depth_buffers();
        }

        void create_mesh_depth_buffers()
        {
                ASSERT(render_buffers_);

                delete_mesh_depth_buffers();

                mesh_renderer_.create_depth_buffers(
                        render_buffers_->framebuffers().size(), {graphics_queue_->family_index()},
                        *graphics_command_pool_, *graphics_queue_, *device_, viewport_.width(), viewport_.height(),
                        renderer_process_.shadow_zoom());
        }

        void create_clear_command_buffers()
        {
                clear_command_buffers_.reset();

                vulkan::CommandBufferCreateInfo info;

                info.device = *device_;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = render_buffers_->width();
                info.render_area->extent.height = render_buffers_->height();
                info.render_pass = render_buffers_->render_pass_clear();
                info.framebuffers = &render_buffers_->framebuffers_clear();
                info.command_pool = *graphics_command_pool_;

                info.before_render_pass_commands = [this](const VkCommandBuffer command_buffer)
                {
                        commands_init_uint32_storage_image(command_buffer, *object_image_, OBJECTS_CLEAR_VALUE);
                };

                const std::vector<VkClearValue> clear_values =
                        render_buffers_->clear_values(renderer_process_.clear_color_rgb32());
                info.clear_values = &clear_values;

                clear_command_buffers_ = vulkan::create_command_buffers(info);
        }

        void create_mesh_render_command_buffers()
        {
                mesh_renderer_.delete_render_command_buffers();

                mesh_renderer_.create_render_command_buffers(
                        renderer_objects_.mesh_visible_objects(), *graphics_command_pool_,
                        renderer_process_.clip_plane().has_value(), renderer_process_.show_normals(),
                        [this](const VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_->commands_init(command_buffer);
                        },
                        [this](const VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_->commands_read(command_buffer);
                        });
        }

        void create_mesh_depth_command_buffers()
        {
                mesh_renderer_.delete_depth_command_buffers();

                mesh_renderer_.create_depth_command_buffers(
                        renderer_objects_.mesh_visible_objects(), *graphics_command_pool_,
                        renderer_process_.clip_plane().has_value(), renderer_process_.show_normals());
        }

        void create_mesh_command_buffers()
        {
                create_mesh_render_command_buffers();
                create_mesh_depth_command_buffers();
        }

        void create_volume_command_buffers()
        {
                volume_renderer_.delete_command_buffers();
                if (renderer_objects_.volume_visible_objects().size() != 1)
                {
                        return;
                }
                const auto copy_depth = [this](const VkCommandBuffer command_buffer)
                {
                        ASSERT(render_buffers_);
                        ASSERT(render_buffers_->framebuffers().size() == 1);
                        constexpr int INDEX = 0;

                        render_buffers_->commands_depth_copy(
                                command_buffer, depth_copy_image_->image(), DEPTH_COPY_IMAGE_LAYOUT, viewport_, INDEX);
                };
                for (const VolumeObject* const visible_volume : renderer_objects_.volume_visible_objects())
                {
                        volume_renderer_.create_command_buffers(visible_volume, *graphics_command_pool_, copy_depth);
                }
        }

        void set_volume_matrix()
        {
                for (VolumeObject* const visible_volume : renderer_objects_.volume_visible_objects())
                {
                        visible_volume->set_matrix_and_clip_plane(
                                renderer_process_.main_vp_matrix(), renderer_process_.clip_plane());
                }
        }

        void event(const StorageMeshCreate& v)
        {
                *v.ptr = create_mesh_object(
                        device_, {graphics_queue_->family_index()}, transfer_command_pool_, transfer_queue_,
                        mesh_layouts_, mesh_material_layouts_, mesh_renderer_.texture_sampler());
        }

        void event(const StorageMeshVisibilityChanged&)
        {
                create_mesh_command_buffers();
        }

        void event(const StorageMeshChanged& v)
        {
                if (v.update_changes->command_buffers || v.update_changes->transparency)
                {
                        create_mesh_command_buffers();
                }
        }

        void event(const StorageVolumeCreate& v)
        {
                *v.ptr = create_volume_object(
                        device_, {graphics_queue_->family_index()}, transfer_command_pool_, transfer_queue_,
                        volume_image_layouts_, volume_renderer_.image_sampler(),
                        volume_renderer_.transfer_function_sampler());
        }

        void event(const StorageVolumeVisibilityChanged&)
        {
                create_volume_command_buffers();
                set_volume_matrix();
        }

        void event(const StorageVolumeChanged& v)
        {
                if (v.update_changes->command_buffers)
                {
                        create_volume_command_buffers();
                }
        }

        // RendererProcessEvents

        void background_changed() override
        {
                create_clear_command_buffers();
        }

        void show_normals_changed() override
        {
                create_mesh_render_command_buffers();
        }

        void shadow_zoom_changed() override
        {
                create_mesh_depth_buffers();
                create_mesh_command_buffers();
        }

        void matrices_changed() override
        {
                set_volume_matrix();
        }

        void clip_plane_changed() override
        {
                create_mesh_render_command_buffers();
                if (renderer_process_.clip_plane())
                {
                        for (VolumeObject* const visible_volume : renderer_objects_.volume_visible_objects())
                        {
                                visible_volume->set_clip_plane(*renderer_process_.clip_plane());
                        }
                }
        }

public:
        Impl(const vulkan::DeviceInstance* const instance,
             const vulkan::CommandPool* const graphics_command_pool,
             const vulkan::Queue* const graphics_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             const bool sample_shading,
             const bool sampler_anisotropy)
                : instance_(instance),
                  device_(&instance->device()),
                  graphics_command_pool_(graphics_command_pool),
                  graphics_queue_(graphics_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  shader_buffers_(*device_, {graphics_queue_->family_index()}),
                  ggx_f1_albedo_(
                          *device_,
                          {graphics_queue_->family_index()},
                          *transfer_command_pool_,
                          *transfer_queue_),
                  mesh_renderer_(device_, sample_shading, sampler_anisotropy, shader_buffers_, ggx_f1_albedo_),
                  volume_renderer_(device_, sample_shading, shader_buffers_, ggx_f1_albedo_),
                  renderer_objects_(
                          [this](const StorageMeshEvents& events)
                          {
                                  const auto visitor = [this](const auto& v)
                                  {
                                          event(v);
                                  };
                                  std::visit(visitor, events);
                          },
                          [this](const StorageVolumeEvents& events)
                          {
                                  const auto visitor = [this](const auto& v)
                                  {
                                          event(v);
                                  };
                                  std::visit(visitor, events);
                          }),
                  renderer_process_(&shader_buffers_, this),
                  renderer_draw_(*device_, TRANSPARENCY_NODE_BUFFER_MAX_SIZE, &mesh_renderer_, &volume_renderer_)
        {
        }

        ~Impl() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                instance_->device_wait_idle_noexcept("the Vulkan renderer destructor");
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

vulkan::DeviceFunctionality Renderer::device_functionality()
{
        vulkan::DeviceFunctionality res;
        res.required_features.features_10.geometryShader = VK_TRUE;
        res.required_features.features_10.fragmentStoresAndAtomics = VK_TRUE;
        res.required_features.features_10.shaderStorageImageMultisample = VK_TRUE;
        res.required_features.features_10.shaderClipDistance = VK_TRUE;
        return res;
}

std::unique_ptr<Renderer> create_renderer(
        const vulkan::DeviceInstance* const instance,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const bool sample_shading,
        const bool sampler_anisotropy)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading,
                sampler_anisotropy);
}
}
