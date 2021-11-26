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

#include "renderer.h"

#include "commands.h"
#include "depth_buffer.h"
#include "mesh_object.h"
#include "mesh_renderer.h"
#include "storage.h"
#include "volume_object.h"
#include "volume_renderer.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/query.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>

#include <memory>
#include <optional>
#include <thread>

namespace ns::gpu::renderer
{
namespace
{
constexpr VkImageLayout DEPTH_COPY_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
constexpr uint32_t OBJECTS_CLEAR_VALUE = 0;
constexpr uint32_t TRANSPARENCY_NODE_BUFFER_MAX_SIZE = (1ull << 30);

vulkan::DeviceFeatures device_features()
{
        vulkan::DeviceFeatures features{};
        features.features_10.geometryShader = VK_TRUE;
        features.features_10.fragmentStoresAndAtomics = VK_TRUE;
        features.features_10.shaderStorageImageMultisample = VK_TRUE;
        features.features_10.shaderClipDistance = VK_TRUE;
        return features;
}

struct ViewportTransform
{
        // device_coordinates = (framebuffer_coordinates - center) * factor
        Vector2d center;
        Vector2d factor;
};
ViewportTransform viewport_transform(const Region<2, int>& viewport)
{
        const Vector2d offset = to_vector<double>(viewport.from());
        const Vector2d extent = to_vector<double>(viewport.extent());
        ViewportTransform t;
        t.center = offset + 0.5 * extent;
        t.factor = Vector2d(2.0 / extent[0], 2.0 / extent[1]);
        return t;
}

void transparency_message(long long required_node_memory, long long overload_count)
{
        thread_local long long previous_required_node_memory = -1;
        thread_local long long previous_overload_count = -1;
        if (required_node_memory < 0)
        {
                if (previous_required_node_memory >= 0)
                {
                        LOG("Transparency memory: OK");
                }
        }
        else
        {
                if (previous_required_node_memory != required_node_memory)
                {
                        std::ostringstream oss;
                        oss << "Transparency memory: required " << required_node_memory / 1'000'000 << " MB, limit "
                            << TRANSPARENCY_NODE_BUFFER_MAX_SIZE / 1'000'000 << " MB.";
                        LOG(oss.str());
                }
        }
        if (overload_count < 0)
        {
                if (previous_overload_count >= 0)
                {
                        LOG("Transparency overload: OK");
                }
        }
        else
        {
                if (previous_overload_count != overload_count)
                {
                        std::ostringstream oss;
                        oss << "Transparency overload: " << overload_count << " samples.";
                        LOG(oss.str());
                }
        }
        previous_required_node_memory = required_node_memory;
        previous_overload_count = overload_count;
}

class Impl final : public Renderer
{
        // shadow coordinates x(-1, 1) y(-1, 1) z(0, 1).
        // shadow texture coordinates x(0, 1) y(0, 1) z(0, 1).
        static constexpr Matrix4d SHADOW_TEXTURE_MATRIX =
                matrix::scale<double>(0.5, 0.5, 1) * matrix::translate<double>(1, 1, 0);

        const std::thread::id thread_id_ = std::this_thread::get_id();

        Matrix4d main_vp_matrix_ = Matrix4d(1);
        Matrix4d shadow_vp_matrix_ = Matrix4d(1);
        Matrix4d shadow_vp_texture_matrix_ = Matrix4d(1);

        Vector3f clear_color_rgb32_ = Vector3f(0);
        double shadow_zoom_ = 1;
        bool show_shadow_ = false;
        Region<2, int> viewport_;
        std::optional<Vector4d> clip_plane_;
        bool show_normals_ = false;

        const vulkan::VulkanInstance* const instance_;
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const RenderBuffers3D* render_buffers_ = nullptr;
        const vulkan::ImageWithMemory* object_image_ = nullptr;

        ShaderBuffers shader_buffers_;
        vulkan::handle::Semaphore renderer_mesh_signal_semaphore_;
        vulkan::handle::Semaphore renderer_volume_signal_semaphore_;

        std::unique_ptr<vulkan::DepthImageWithMemory> depth_copy_image_;

        std::unique_ptr<DepthBuffers> mesh_renderer_depth_render_buffers_;
        vulkan::handle::Semaphore mesh_renderer_depth_signal_semaphore_;
        MeshRenderer mesh_renderer_;

        vulkan::handle::Semaphore volume_renderer_signal_semaphore_;
        VolumeRenderer volume_renderer_;

        ObjectStorage<MeshObject> mesh_storage_;
        ObjectStorage<VolumeObject> volume_storage_;

        std::optional<vulkan::handle::CommandBuffers> clear_command_buffers_;
        vulkan::handle::Semaphore clear_signal_semaphore_;

        std::unique_ptr<TransparencyBuffers> transparency_buffers_;
        vulkan::handle::Semaphore render_transparent_as_opaque_signal_semaphore_;

        void set_lighting_color(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_lighting_color(color.rgb32().max_n(0));
        }

        void set_background_color(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                clear_color_rgb32_ = color.rgb32().clamp(0, 1);
                shader_buffers_.set_background_color(clear_color_rgb32_);

                create_clear_command_buffers();
        }

        void set_wireframe_color(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_wireframe_color(color.rgb32().clamp(0, 1));
        }

        void set_clip_plane_color(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_clip_plane_color(color.rgb32().clamp(0, 1));
        }

        void set_normal_length(const float length) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_normal_length(length);
        }

        void set_normal_color_positive(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_normal_color_positive(color.rgb32().clamp(0, 1));
        }

        void set_normal_color_negative(const color::Color& color) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_normal_color_negative(color.rgb32().clamp(0, 1));
        }

        void set_show_smooth(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_show_smooth(show);
        }

        void set_show_wireframe(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_show_wireframe(show);
        }

        void set_show_shadow(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_show_shadow(show);
                show_shadow_ = show;
        }

        void set_show_fog(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_show_fog(show);
        }

        void set_show_materials(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shader_buffers_.set_show_materials(show);
        }

        void set_show_normals(const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                if (show_normals_ != show)
                {
                        show_normals_ = show;
                        create_mesh_render_command_buffers();
                }
        }

        void set_shadow_zoom(const double zoom) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                shadow_zoom_ = zoom;

                create_mesh_depth_buffers();
                create_mesh_command_buffers();
        }

        void set_camera(const CameraInfo& c) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                const Matrix4d& shadow_projection_matrix = matrix::ortho_vulkan<double>(
                        c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom, c.shadow_volume.top,
                        c.shadow_volume.near, c.shadow_volume.far);
                const Matrix4d& main_projection_matrix = matrix::ortho_vulkan<double>(
                        c.main_volume.left, c.main_volume.right, c.main_volume.bottom, c.main_volume.top,
                        c.main_volume.near, c.main_volume.far);

                shadow_vp_matrix_ = shadow_projection_matrix * c.shadow_view_matrix;
                shadow_vp_texture_matrix_ = SHADOW_TEXTURE_MATRIX * shadow_vp_matrix_;
                main_vp_matrix_ = main_projection_matrix * c.main_view_matrix;

                shader_buffers_.set_direction_to_light(-to_vector<float>(c.light_direction));
                shader_buffers_.set_direction_to_camera(-to_vector<float>(c.camera_direction));

                set_matrices();
        }

        void set_clip_plane(const std::optional<Vector4d>& plane) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                clip_plane_ = plane;
                if (clip_plane_)
                {
                        shader_buffers_.set_clip_plane(*clip_plane_, true);

                        for (VolumeObject* visible_volume : volume_storage_.visible_objects())
                        {
                                visible_volume->set_clip_plane(*clip_plane_);
                        }
                }
                else
                {
                        shader_buffers_.set_clip_plane(Vector4d(0), false);
                }
                create_mesh_render_command_buffers();
        }

        void object_update(const mesh::MeshObject<3>& object) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(volume_storage_.find(object.id()) == nullptr);

                bool created = false;

                MeshObject* ptr = mesh_storage_.find(object.id());
                if (!ptr)
                {
                        ptr = mesh_storage_.insert(
                                object.id(),
                                create_mesh_object(
                                        device_, {graphics_queue_->family_index()}, transfer_command_pool_,
                                        transfer_queue_, mesh_renderer_.mesh_layouts(),
                                        mesh_renderer_.material_layouts(), mesh_renderer_.texture_sampler()));

                        created = true;
                }

                MeshObject::UpdateChanges update_changes;
                try
                {
                        mesh::Reading reading(object);
                        update_changes = ptr->update(reading);
                }
                catch (const std::exception& e)
                {
                        mesh_storage_.erase(object.id());
                        LOG(std::string("Error updating mesh object. ") + e.what());
                        return;
                }
                catch (...)
                {
                        mesh_storage_.erase(object.id());
                        LOG("Unknown error updating mesh object");
                        return;
                }

                ASSERT(!(created && mesh_storage_.is_visible(object.id())));

                if ((update_changes.command_buffers || update_changes.transparency)
                    && mesh_storage_.is_visible(object.id()))
                {
                        create_mesh_command_buffers();
                }

                if (created)
                {
                        object_show(object.id(), object.visible());
                }
        }

        void object_update(const volume::VolumeObject<3>& object) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(mesh_storage_.find(object.id()) == nullptr);

                bool created = false;

                VolumeObject* ptr = volume_storage_.find(object.id());
                if (!ptr)
                {
                        ptr = volume_storage_.insert(
                                object.id(), create_volume_object(
                                                     device_, {graphics_queue_->family_index()}, transfer_command_pool_,
                                                     transfer_queue_, volume_renderer_.image_layouts(),
                                                     volume_renderer_.image_sampler(),
                                                     volume_renderer_.transfer_function_sampler()));

                        created = true;
                }

                VolumeObject::UpdateChanges update_changes;
                try
                {
                        volume::Reading reading(object);
                        update_changes = ptr->update(reading);
                }
                catch (const std::exception& e)
                {
                        volume_storage_.erase(object.id());
                        LOG(std::string("Error updating volume object. ") + e.what());
                        return;
                }
                catch (...)
                {
                        volume_storage_.erase(object.id());
                        LOG("Unknown error updating volume object");
                        return;
                }

                ASSERT(!(created && volume_storage_.is_visible(object.id())));

                if (update_changes.command_buffers && volume_storage_.is_visible(object.id()))
                {
                        create_volume_command_buffers();
                }

                if (created)
                {
                        object_show(object.id(), object.visible());
                }
        }

        void object_delete(const ObjectId id) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                if (mesh_storage_.erase(id))
                {
                        return;
                }
                if (volume_storage_.erase(id))
                {
                        return;
                }
        }

        void object_delete_all() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                mesh_storage_.clear();
                volume_storage_.clear();
        }

        void object_show(const ObjectId id, const bool show) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                if (mesh_storage_.set_visible(id, show))
                {
                        return;
                }
                if (volume_storage_.set_visible(id, show))
                {
                        return;
                }
        }

        [[nodiscard]] std::tuple<VkSemaphore, bool> draw_meshes(
                VkSemaphore semaphore,
                const vulkan::Queue& graphics_queue,
                const unsigned index) const
        {
                if (!show_shadow_)
                {
                        ASSERT(*mesh_renderer_.render_command_buffer_all(index));
                        vulkan::queue_submit(
                                semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                *mesh_renderer_.render_command_buffer_all(index), renderer_mesh_signal_semaphore_,
                                graphics_queue);

                        semaphore = renderer_mesh_signal_semaphore_;
                }
                else
                {
                        ASSERT(mesh_renderer_.depth_command_buffer(index));
                        vulkan::queue_submit(
                                *mesh_renderer_.depth_command_buffer(index), mesh_renderer_depth_signal_semaphore_,
                                graphics_queue);

                        ASSERT(*mesh_renderer_.render_command_buffer_all(index));
                        vulkan::queue_submit(
                                std::array<VkSemaphore, 2>{semaphore, mesh_renderer_depth_signal_semaphore_},
                                std::array<VkPipelineStageFlags, 2>{
                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
                                *mesh_renderer_.render_command_buffer_all(index), renderer_mesh_signal_semaphore_,
                                graphics_queue);

                        semaphore = renderer_mesh_signal_semaphore_;
                }

                if (!mesh_renderer_.has_transparent_meshes())
                {
                        transparency_message(-1, -1);
                        return {semaphore, false /*transparency*/};
                }

                vulkan::queue_wait_idle(graphics_queue);

                unsigned long long required_node_memory;
                unsigned overload_counter;
                transparency_buffers_->read(&required_node_memory, &overload_counter);

                const bool nodes = required_node_memory > TRANSPARENCY_NODE_BUFFER_MAX_SIZE;
                const bool overload = overload_counter > 0;
                bool transparency;
                if (nodes || overload)
                {
                        ASSERT(*mesh_renderer_.render_command_buffer_transparent_as_opaque(index));
                        vulkan::queue_submit(
                                semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                *mesh_renderer_.render_command_buffer_transparent_as_opaque(index),
                                render_transparent_as_opaque_signal_semaphore_, graphics_queue);

                        semaphore = render_transparent_as_opaque_signal_semaphore_;
                        transparency = false;
                }
                else
                {
                        transparency = true;
                }

                transparency_message(
                        nodes ? static_cast<long long>(required_node_memory) : -1,
                        overload ? static_cast<long long>(overload_counter) : -1);

                return {semaphore, transparency};
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
                ASSERT(index < clear_command_buffers_->count());
                vulkan::queue_submit((*clear_command_buffers_)[index], clear_signal_semaphore_, graphics_queue_2);

                VkSemaphore semaphore = clear_signal_semaphore_;

                bool transparency = false;

                if (mesh_renderer_.has_meshes())
                {
                        std::tie(semaphore, transparency) = draw_meshes(semaphore, graphics_queue_1, index);
                }

                if (volume_renderer_.has_volume() || transparency)
                {
                        ASSERT(*volume_renderer_.command_buffer(index, transparency));
                        vulkan::queue_submit(
                                semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                *volume_renderer_.command_buffer(index, transparency),
                                renderer_volume_signal_semaphore_, graphics_queue_1);

                        semaphore = renderer_volume_signal_semaphore_;
                }

                return semaphore;
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

                ViewportTransform t = viewport_transform(viewport_);
                shader_buffers_.set_viewport(t.center, t.factor);

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

                //

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
                        *device_, std::vector<uint32_t>({graphics_queue_->family_index()}),
                        std::vector<VkFormat>({render_buffers_->depth_format()}), render_buffers_->sample_count(),
                        render_buffers_->width(), render_buffers_->height(),
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, DEPTH_COPY_IMAGE_LAYOUT,
                        *graphics_command_pool_, *graphics_queue_);
        }

        void create_transparency_buffers()
        {
                transparency_buffers_ = std::make_unique<TransparencyBuffers>(
                        *device_, *graphics_command_pool_, *graphics_queue_,
                        std::vector<uint32_t>({graphics_queue_->family_index()}), render_buffers_->sample_count(),
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
                mesh_renderer_depth_render_buffers_.reset();
        }

        void create_mesh_depth_buffers()
        {
                ASSERT(render_buffers_);

                delete_mesh_depth_buffers();

                mesh_renderer_depth_render_buffers_ = create_depth_buffers(
                        render_buffers_->framebuffers().size(), {graphics_queue_->family_index()},
                        *graphics_command_pool_, *graphics_queue_, *device_, viewport_.width(), viewport_.height(),
                        shadow_zoom_);

                mesh_renderer_.create_depth_buffers(mesh_renderer_depth_render_buffers_.get());
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

                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer)
                {
                        commands_init_uint32_storage_image(command_buffer, *object_image_, OBJECTS_CLEAR_VALUE);
                };

                const std::vector<VkClearValue> clear_values = render_buffers_->clear_values(clear_color_rgb32_);
                info.clear_values = &clear_values;

                clear_command_buffers_ = vulkan::create_command_buffers(info);
        }

        void create_mesh_render_command_buffers()
        {
                mesh_renderer_.delete_render_command_buffers();

                mesh_renderer_.create_render_command_buffers(
                        mesh_storage_.visible_objects(), *graphics_command_pool_, clip_plane_.has_value(),
                        show_normals_,
                        [this](VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_->commands_init(command_buffer);
                        },
                        [this](VkCommandBuffer command_buffer)
                        {
                                transparency_buffers_->commands_read(command_buffer);
                        });
        }

        void create_mesh_depth_command_buffers()
        {
                mesh_renderer_.delete_depth_command_buffers();
                mesh_renderer_.create_depth_command_buffers(
                        mesh_storage_.visible_objects(), *graphics_command_pool_, clip_plane_.has_value(),
                        show_normals_);
        }

        void create_mesh_command_buffers()
        {
                create_mesh_render_command_buffers();
                create_mesh_depth_command_buffers();
        }

        void create_volume_command_buffers()
        {
                volume_renderer_.delete_command_buffers();
                if (volume_storage_.visible_objects().size() != 1)
                {
                        return;
                }
                auto copy_depth = [this](VkCommandBuffer command_buffer)
                {
                        ASSERT(render_buffers_);
                        ASSERT(render_buffers_->framebuffers().size() == 1);
                        constexpr int INDEX = 0;

                        render_buffers_->commands_depth_copy(
                                command_buffer, depth_copy_image_->image(), DEPTH_COPY_IMAGE_LAYOUT, viewport_, INDEX);
                };
                for (VolumeObject* visible_volume : volume_storage_.visible_objects())
                {
                        volume_renderer_.create_command_buffers(visible_volume, *graphics_command_pool_, copy_depth);
                }
        }

        void set_volume_matrix()
        {
                for (VolumeObject* visible_volume : volume_storage_.visible_objects())
                {
                        visible_volume->set_matrix_and_clip_plane(main_vp_matrix_, clip_plane_);
                }
        }

        void set_matrices()
        {
                shader_buffers_.set_matrices(main_vp_matrix_, shadow_vp_matrix_, shadow_vp_texture_matrix_);
                set_volume_matrix();
        }

        void mesh_visibility_changed()
        {
                create_mesh_command_buffers();
        }

        void volume_visibility_changed()
        {
                create_volume_command_buffers();
                set_volume_matrix();
        }

public:
        Impl(const vulkan::VulkanInstance* const instance,
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
                  renderer_mesh_signal_semaphore_(*device_),
                  renderer_volume_signal_semaphore_(*device_),
                  mesh_renderer_depth_signal_semaphore_(*device_),
                  mesh_renderer_(device_, sample_shading, sampler_anisotropy, shader_buffers_),
                  volume_renderer_signal_semaphore_(*device_),
                  volume_renderer_(device_, sample_shading, shader_buffers_),
                  mesh_storage_(
                          [this]()
                          {
                                  mesh_visibility_changed();
                          }),
                  volume_storage_(
                          [this]()
                          {
                                  volume_visibility_changed();
                          }),
                  clear_signal_semaphore_(*device_),
                  render_transparent_as_opaque_signal_semaphore_(*device_)
        {
        }

        ~Impl() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                instance_->device_wait_idle_noexcept("the Vulkan renderer destructor");
        }
};
}

vulkan::DeviceFeatures Renderer::required_device_features()
{
        return device_features();
}

std::unique_ptr<Renderer> create_renderer(
        const vulkan::VulkanInstance* const instance,
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
