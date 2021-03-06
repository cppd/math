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
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::fragmentStoresAndAtomics,
        vulkan::PhysicalDeviceFeatures::geometryShader,
        vulkan::PhysicalDeviceFeatures::shaderClipDistance,
        vulkan::PhysicalDeviceFeatures::shaderStorageImageMultisample
};
// clang-format on

constexpr VkImageLayout DEPTH_COPY_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
constexpr uint32_t OBJECTS_CLEAR_VALUE = 0;
constexpr uint32_t TRANSPARENCY_NODE_BUFFER_MAX_SIZE = (1ull << 30);

struct ViewportTransform
{
        // device_coordinates = (framebuffer_coordinates - center) * factor
        vec2d center;
        vec2d factor;
};
ViewportTransform viewport_transform(const Region<2, int>& viewport)
{
        const vec2d offset = to_vector<double>(viewport.from());
        const vec2d extent = to_vector<double>(viewport.extent());
        ViewportTransform t;
        t.center = offset + 0.5 * extent;
        t.factor = vec2d(2.0 / extent[0], 2.0 / extent[1]);
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
        // Для получения текстуры для тени результат рисования находится в интервалах x(-1, 1) y(-1, 1) z(0, 1).
        // Для работы с этой текстурой надо преобразовать в интервалы x(0, 1) y(0, 1) z(0, 1).
        const mat4d SHADOW_TEXTURE_MATRIX = matrix::scale<double>(0.5, 0.5, 1) * matrix::translate<double>(1, 1, 0);

        const std::thread::id m_thread_id = std::this_thread::get_id();

        mat4d m_main_vp_matrix = mat4d(1);
        mat4d m_shadow_vp_matrix = mat4d(1);
        mat4d m_shadow_vp_texture_matrix = mat4d(1);

        Color m_clear_color = Color(0);
        double m_shadow_zoom = 1;
        bool m_show_shadow = false;
        Region<2, int> m_viewport;
        std::optional<vec4d> m_clip_plane;
        bool m_show_normals = false;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;
        const vulkan::CommandPool& m_transfer_command_pool;
        const vulkan::Queue& m_transfer_queue;

        const vulkan::Swapchain* m_swapchain = nullptr;
        const RenderBuffers3D* m_render_buffers = nullptr;
        const vulkan::ImageWithMemory* m_object_image = nullptr;

        ShaderBuffers m_shader_buffers;
        vulkan::Semaphore m_renderer_mesh_signal_semaphore;
        vulkan::Semaphore m_renderer_volume_signal_semaphore;

        std::unique_ptr<vulkan::DepthImageWithMemory> m_depth_copy_image;

        std::unique_ptr<DepthBuffers> m_mesh_renderer_depth_render_buffers;
        vulkan::Semaphore m_mesh_renderer_depth_signal_semaphore;
        MeshRenderer m_mesh_renderer;

        vulkan::Semaphore m_volume_renderer_signal_semaphore;
        VolumeRenderer m_volume_renderer;

        ObjectStorage<MeshObject> m_mesh_storage;
        ObjectStorage<VolumeObject> m_volume_storage;

        std::optional<vulkan::CommandBuffers> m_clear_command_buffers;
        vulkan::Semaphore m_clear_signal_semaphore;

        std::unique_ptr<TransparencyBuffers> m_transparency_buffers;
        vulkan::Semaphore m_render_transparent_as_opaque_signal_semaphore;

        void set_lighting_intensity(double intensity) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_lighting_intensity(intensity);
        }
        void set_background_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clear_color = color;
                m_shader_buffers.set_background_color(color);

                create_clear_command_buffers();
        }
        void set_wireframe_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_wireframe_color(color);
        }
        void set_clip_plane_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_clip_plane_color(color);
        }
        void set_normal_length(float length) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_normal_length(length);
        }
        void set_normal_color_positive(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_normal_color_positive(color);
        }
        void set_normal_color_negative(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_normal_color_negative(color);
        }
        void set_show_smooth(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_show_smooth(show);
        }
        void set_show_wireframe(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_show_wireframe(show);
        }
        void set_show_shadow(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_show_shadow(show);
                m_show_shadow = show;
        }
        void set_show_fog(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_show_fog(show);
        }
        void set_show_materials(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_show_materials(show);
        }
        void set_show_normals(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_show_normals != show)
                {
                        m_show_normals = show;
                        create_mesh_render_command_buffers();
                }
        }
        void set_shadow_zoom(double zoom) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shadow_zoom = zoom;

                create_mesh_depth_buffers();
                create_mesh_command_buffers();
        }
        void set_camera(const CameraInfo& c) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                const mat4d& shadow_projection_matrix = matrix::ortho_vulkan<double>(
                        c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom, c.shadow_volume.top,
                        c.shadow_volume.near, c.shadow_volume.far);
                const mat4d& main_projection_matrix = matrix::ortho_vulkan<double>(
                        c.main_volume.left, c.main_volume.right, c.main_volume.bottom, c.main_volume.top,
                        c.main_volume.near, c.main_volume.far);

                m_shadow_vp_matrix = shadow_projection_matrix * c.shadow_view_matrix;
                m_shadow_vp_texture_matrix = SHADOW_TEXTURE_MATRIX * m_shadow_vp_matrix;
                m_main_vp_matrix = main_projection_matrix * c.main_view_matrix;

                m_shader_buffers.set_direction_to_light(-to_vector<float>(c.light_direction));
                m_shader_buffers.set_direction_to_camera(-to_vector<float>(c.camera_direction));

                set_matrices();
        }

        void set_clip_plane(const std::optional<vec4d>& plane) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clip_plane = plane;
                if (m_clip_plane)
                {
                        m_shader_buffers.set_clip_plane(*m_clip_plane, true);

                        for (VolumeObject* visible_volume : m_volume_storage.visible_objects())
                        {
                                visible_volume->set_clip_plane(*m_clip_plane);
                        }
                }
                else
                {
                        m_shader_buffers.set_clip_plane(vec4d(0), false);
                }
                create_mesh_render_command_buffers();
        }

        void object_update(const mesh::MeshObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                bool created = false;

                {
                        mesh::Reading reading(object);

                        //

                        ASSERT(m_volume_storage.find(object.id()) == nullptr);

                        MeshObject* ptr = m_mesh_storage.find(object.id());
                        if (!ptr)
                        {
                                ptr = m_mesh_storage.insert(
                                        object.id(),
                                        create_mesh_object(
                                                m_device, m_graphics_command_pool, m_graphics_queue,
                                                m_transfer_command_pool, m_transfer_queue,
                                                m_mesh_renderer.mesh_layouts(), m_mesh_renderer.material_layouts(),
                                                m_mesh_renderer.texture_sampler()));

                                created = true;
                        }

                        MeshObject::UpdateChanges update_changes;
                        try
                        {
                                update_changes = ptr->update(reading);
                        }
                        catch (const std::exception& e)
                        {
                                m_mesh_storage.erase(object.id());
                                LOG(std::string("Error updating mesh object. ") + e.what());
                                return;
                        }
                        catch (...)
                        {
                                m_mesh_storage.erase(object.id());
                                LOG("Unknown error updating mesh object");
                                return;
                        }

                        ASSERT(!(created && m_mesh_storage.is_visible(object.id())));
                        if ((update_changes.command_buffers || update_changes.transparency)
                            && m_mesh_storage.is_visible(object.id()))
                        {
                                create_mesh_command_buffers();
                        }
                }

                if (created)
                {
                        object_show(object.id(), object.visible());
                }
        }

        void object_update(const volume::VolumeObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                bool created = false;

                {
                        volume::Reading reading(object);

                        //

                        ASSERT(m_mesh_storage.find(object.id()) == nullptr);

                        VolumeObject* ptr = m_volume_storage.find(object.id());
                        if (!ptr)
                        {
                                ptr = m_volume_storage.insert(
                                        object.id(),
                                        create_volume_object(
                                                m_device, m_graphics_command_pool, m_graphics_queue,
                                                m_transfer_command_pool, m_transfer_queue,
                                                m_volume_renderer.image_layouts(), m_volume_renderer.image_sampler(),
                                                m_volume_renderer.transfer_function_sampler()));

                                created = true;
                        }

                        VolumeObject::UpdateChanges update_changes;
                        try
                        {
                                update_changes = ptr->update(reading);
                        }
                        catch (const std::exception& e)
                        {
                                m_volume_storage.erase(object.id());
                                LOG(std::string("Error updating volume object. ") + e.what());
                                return;
                        }
                        catch (...)
                        {
                                m_volume_storage.erase(object.id());
                                LOG("Unknown error updating volume object");
                                return;
                        }

                        ASSERT(!(created && m_volume_storage.is_visible(object.id())));
                        if (update_changes.command_buffers && m_volume_storage.is_visible(object.id()))
                        {
                                create_volume_command_buffers();
                        }
                }

                if (created)
                {
                        object_show(object.id(), object.visible());
                }
        }

        void object_delete(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (m_mesh_storage.erase(id))
                {
                        return;
                }
                if (m_volume_storage.erase(id))
                {
                        return;
                }
        }

        void object_delete_all() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_mesh_storage.clear();
                m_volume_storage.clear();
        }

        void object_show(ObjectId id, bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (m_mesh_storage.set_visible(id, show))
                {
                        return;
                }
                if (m_volume_storage.set_visible(id, show))
                {
                        return;
                }
        }

        VkSemaphore draw(
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                unsigned image_index) const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(graphics_queue_1.family_index() == m_graphics_queue.family_index());
                ASSERT(graphics_queue_2.family_index() == m_graphics_queue.family_index());

                ASSERT(image_index < m_swapchain->image_views().size());

                VkSemaphore semaphore;

                {
                        ASSERT(m_clear_command_buffers);
                        unsigned index = m_clear_command_buffers->count() == 1 ? 0 : image_index;
                        vulkan::queue_submit(
                                (*m_clear_command_buffers)[index], m_clear_signal_semaphore, graphics_queue_2);
                        semaphore = m_clear_signal_semaphore;
                }

                bool transparency = false;
                if (m_mesh_renderer.has_meshes())
                {
                        if (!m_show_shadow)
                        {
                                vulkan::queue_submit(
                                        semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        *m_mesh_renderer.render_command_buffer_all(image_index),
                                        m_renderer_mesh_signal_semaphore, graphics_queue_1);

                                semaphore = m_renderer_mesh_signal_semaphore;
                        }
                        else
                        {
                                ASSERT(m_mesh_renderer.depth_command_buffer(image_index));
                                vulkan::queue_submit(
                                        *m_mesh_renderer.depth_command_buffer(image_index),
                                        m_mesh_renderer_depth_signal_semaphore, graphics_queue_1);

                                vulkan::queue_submit(
                                        std::array<VkSemaphore, 2>{semaphore, m_mesh_renderer_depth_signal_semaphore},
                                        std::array<VkPipelineStageFlags, 2>{
                                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
                                        *m_mesh_renderer.render_command_buffer_all(image_index),
                                        m_renderer_mesh_signal_semaphore, graphics_queue_1);

                                semaphore = m_renderer_mesh_signal_semaphore;
                        }

                        if (m_mesh_renderer.has_transparent_meshes())
                        {
                                vulkan::queue_wait_idle(graphics_queue_1);

                                unsigned long long required_node_memory;
                                unsigned overload_counter;
                                m_transparency_buffers->read(&required_node_memory, &overload_counter);

                                bool nodes = required_node_memory > TRANSPARENCY_NODE_BUFFER_MAX_SIZE;
                                bool overload = overload_counter > 0;
                                if (nodes || overload)
                                {
                                        vulkan::queue_submit(
                                                semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                                *m_mesh_renderer.render_command_buffer_transparent_as_opaque(
                                                        image_index),
                                                m_render_transparent_as_opaque_signal_semaphore, graphics_queue_1);

                                        semaphore = m_render_transparent_as_opaque_signal_semaphore;
                                }
                                else
                                {
                                        transparency = true;
                                }

                                transparency_message(
                                        nodes ? static_cast<long long>(required_node_memory) : -1,
                                        overload ? static_cast<long long>(overload_counter) : -1);
                        }
                        else
                        {
                                transparency_message(-1, -1);
                        }
                }

                if (m_volume_renderer.has_volume() || transparency)
                {
                        vulkan::queue_submit(
                                semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                *m_volume_renderer.command_buffer(image_index, transparency),
                                m_renderer_volume_signal_semaphore, graphics_queue_1);

                        semaphore = m_renderer_volume_signal_semaphore;
                }

                return semaphore;
        }

        bool empty() const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                return !m_mesh_renderer.render_command_buffer_all(0).has_value() && !m_volume_renderer.has_volume();
        }

        void create_buffers(
                const vulkan::Swapchain* swapchain,
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                const Region<2, int>& viewport) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(swapchain);
                ASSERT(viewport.x1() <= static_cast<int>(objects->width()));
                ASSERT(viewport.y1() <= static_cast<int>(objects->height()));

                m_swapchain = swapchain;
                m_render_buffers = render_buffers;
                m_object_image = objects;
                m_viewport = viewport;

                ViewportTransform t = viewport_transform(m_viewport);
                m_shader_buffers.set_viewport(t.center, t.factor);

                ASSERT(m_render_buffers->framebuffers().size() == m_render_buffers->framebuffers_clear().size());
                ASSERT(m_render_buffers->framebuffers().size() == 1);

                create_depth_image();
                create_transparency_buffers();

                m_mesh_renderer.create_render_buffers(
                        m_render_buffers, *m_object_image, m_transparency_buffers->heads(),
                        m_transparency_buffers->heads_size(), m_transparency_buffers->counters(),
                        m_transparency_buffers->nodes(), m_viewport);
                create_mesh_depth_buffers();

                m_volume_renderer.create_buffers(
                        m_render_buffers, m_graphics_command_pool, m_viewport, m_depth_copy_image->image_view(),
                        m_transparency_buffers->heads(), m_transparency_buffers->nodes());

                create_mesh_command_buffers();
                create_volume_command_buffers();
                create_clear_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_clear_command_buffers.reset();
                m_volume_renderer.delete_buffers();
                delete_mesh_depth_buffers();
                m_mesh_renderer.delete_render_buffers();
                m_depth_copy_image.reset();
                delete_transparency_buffers();
        }

        void create_depth_image()
        {
                m_depth_copy_image = std::make_unique<vulkan::DepthImageWithMemory>(
                        m_device, std::unordered_set({m_graphics_queue.family_index()}),
                        std::vector<VkFormat>({m_render_buffers->depth_format()}), m_render_buffers->sample_count(),
                        m_swapchain->width(), m_swapchain->height(),
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_graphics_command_pool,
                        m_graphics_queue, DEPTH_COPY_IMAGE_LAYOUT);
        }

        void create_transparency_buffers()
        {
                m_transparency_buffers = std::make_unique<TransparencyBuffers>(
                        m_device, m_graphics_command_pool, m_graphics_queue,
                        std::unordered_set<uint32_t>({m_graphics_queue.family_index()}),
                        m_render_buffers->sample_count(), m_swapchain->width(), m_swapchain->height(),
                        TRANSPARENCY_NODE_BUFFER_MAX_SIZE);

                LOG("Transparency node count: " + to_string_digit_groups(m_transparency_buffers->node_count()));

                m_shader_buffers.set_transparency_max_node_count(m_transparency_buffers->node_count());
        }

        void delete_transparency_buffers()
        {
                m_transparency_buffers.reset();
        }

        void delete_mesh_depth_buffers()
        {
                m_mesh_renderer.delete_depth_buffers();
                m_mesh_renderer_depth_render_buffers.reset();
        }

        void create_mesh_depth_buffers()
        {
                ASSERT(m_swapchain);

                delete_mesh_depth_buffers();

                constexpr DepthBufferCount buffer_count = DepthBufferCount::One;
                m_mesh_renderer_depth_render_buffers = create_depth_buffers(
                        buffer_count, *m_swapchain, {m_graphics_queue.family_index()}, m_graphics_command_pool,
                        m_graphics_queue, m_device, m_viewport.width(), m_viewport.height(), m_shadow_zoom);

                m_mesh_renderer.create_depth_buffers(m_mesh_renderer_depth_render_buffers.get());
        }

        void create_clear_command_buffers()
        {
                m_clear_command_buffers.reset();

                vulkan::CommandBufferCreateInfo info;

                info.device = m_device;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = m_render_buffers->width();
                info.render_area->extent.height = m_render_buffers->height();
                info.render_pass = m_render_buffers->render_pass_clear();
                info.framebuffers = &m_render_buffers->framebuffers_clear();
                info.command_pool = m_graphics_command_pool;

                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer)
                {
                        commands_init_uint32_storage_image(command_buffer, *m_object_image, OBJECTS_CLEAR_VALUE);
                };

                const std::vector<VkClearValue> clear_values = m_render_buffers->clear_values(m_clear_color);
                info.clear_values = &clear_values;

                m_clear_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_mesh_render_command_buffers()
        {
                m_mesh_renderer.delete_render_command_buffers();

                m_mesh_renderer.create_render_command_buffers(
                        m_mesh_storage.visible_objects(), m_graphics_command_pool, m_clip_plane.has_value(),
                        m_show_normals,
                        [this](VkCommandBuffer command_buffer)
                        {
                                m_transparency_buffers->commands_init(command_buffer);
                        },
                        [this](VkCommandBuffer command_buffer)
                        {
                                m_transparency_buffers->commands_read(command_buffer);
                        });
        }

        void create_mesh_depth_command_buffers()
        {
                m_mesh_renderer.delete_depth_command_buffers();
                m_mesh_renderer.create_depth_command_buffers(
                        m_mesh_storage.visible_objects(), m_graphics_command_pool, m_clip_plane.has_value(),
                        m_show_normals);
        }

        void create_mesh_command_buffers()
        {
                create_mesh_render_command_buffers();
                create_mesh_depth_command_buffers();
        }

        void create_volume_command_buffers()
        {
                m_volume_renderer.delete_command_buffers();
                if (m_volume_storage.visible_objects().size() != 1)
                {
                        return;
                }
                auto copy_depth = [this](VkCommandBuffer command_buffer)
                {
                        m_render_buffers->commands_depth_copy(
                                command_buffer, m_depth_copy_image->image(), DEPTH_COPY_IMAGE_LAYOUT, m_viewport,
                                0 /*image_index*/);
                };
                for (VolumeObject* visible_volume : m_volume_storage.visible_objects())
                {
                        m_volume_renderer.create_command_buffers(visible_volume, m_graphics_command_pool, copy_depth);
                }
        }

        void set_volume_matrix()
        {
                for (VolumeObject* visible_volume : m_volume_storage.visible_objects())
                {
                        visible_volume->set_matrix_and_clip_plane(m_main_vp_matrix, m_clip_plane);
                }
        }

        void set_matrices()
        {
                m_shader_buffers.set_matrices(m_main_vp_matrix, m_shadow_vp_matrix, m_shadow_vp_texture_matrix);
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
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             bool sample_shading,
             bool sampler_anisotropy)
                : m_instance(instance),
                  m_device(instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_shader_buffers(m_device, {m_graphics_queue.family_index()}),
                  m_renderer_mesh_signal_semaphore(m_device),
                  m_renderer_volume_signal_semaphore(m_device),
                  m_mesh_renderer_depth_signal_semaphore(m_device),
                  m_mesh_renderer(m_device, sample_shading, sampler_anisotropy, m_shader_buffers),
                  m_volume_renderer_signal_semaphore(m_device),
                  m_volume_renderer(m_device, sample_shading, m_shader_buffers),
                  m_mesh_storage(
                          [this]()
                          {
                                  mesh_visibility_changed();
                          }),
                  m_volume_storage(
                          [this]()
                          {
                                  volume_visibility_changed();
                          }),
                  m_clear_signal_semaphore(m_device),
                  m_render_transparent_as_opaque_signal_semaphore(m_device)
        {
        }

        ~Impl() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_instance.device_wait_idle_noexcept("the Vulkan renderer destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> Renderer::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<Renderer> create_renderer(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading,
        bool sampler_anisotropy)
{
        return std::make_unique<Impl>(
                instance, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, sample_shading,
                sampler_anisotropy);
}
}
