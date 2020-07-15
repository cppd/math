/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "depth_buffer.h"
#include "mesh_object.h"
#include "mesh_renderer.h"
#include "volume_object.h"
#include "volume_renderer.h"

#include <src/com/log.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/query.h>
#include <src/vulkan/queue.h>

#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>

namespace gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::fragmentStoresAndAtomics,
        vulkan::PhysicalDeviceFeatures::geometryShader,
        vulkan::PhysicalDeviceFeatures::shaderClipDistance
};
// clang-format on

template <typename T>
class ObjectStorage
{
        static_assert(std::is_same_v<T, MeshObject> || std::is_same_v<T, VolumeObject>);
        static constexpr bool EXCLUSIVE_VISIBILITY = std::is_same_v<T, VolumeObject>;

        std::unordered_map<ObjectId, std::unique_ptr<T>> m_map;
        std::unordered_set<const T*> m_visible_objects;
        std::function<void()> m_visibility_changed;

public:
        ObjectStorage(std::function<void()>&& visibility_changed) : m_visibility_changed(std::move(visibility_changed))
        {
                ASSERT(m_visibility_changed);
        }

        T* insert(ObjectId id, std::unique_ptr<T>&& object)
        {
                const auto pair = m_map.emplace(id, std::move(object));
                ASSERT(pair.second);
                return pair.first->second.get();
        }

        void erase(ObjectId id)
        {
                auto iter = m_map.find(id);
                if (iter == m_map.cend())
                {
                        return;
                }
                bool visibility_changed = m_visible_objects.erase(iter->second.get()) > 0;
                m_map.erase(iter);
                if (visibility_changed)
                {
                        m_visibility_changed();
                }
        }

        bool empty() const
        {
                ASSERT(!m_map.empty() || m_visible_objects.empty());
                return m_map.empty();
        }

        void clear()
        {
                bool visibility_changed = !m_visible_objects.empty();
                m_visible_objects.clear();
                m_map.clear();
                if (visibility_changed)
                {
                        m_visibility_changed();
                }
        }

        template <typename Id>
        std::enable_if_t<std::is_same_v<Id, ObjectId>, T*> find(const Id& id) const
        {
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.get() : nullptr;
        }

        template <typename OptionalId>
        std::enable_if_t<std::is_same_v<OptionalId, std::optional<ObjectId>>, T*> find(const OptionalId& id) const
        {
                return id ? find(*id) : nullptr;
        }

        bool set_visible(ObjectId id, bool visible)
        {
                auto iter = m_map.find(id);
                if (iter == m_map.cend())
                {
                        return false;
                }

                const T* ptr = iter->second.get();
                auto iter_v = m_visible_objects.find(ptr);
                if (!visible)
                {
                        if (iter_v != m_visible_objects.cend())
                        {
                                m_visible_objects.erase(iter_v);
                                m_visibility_changed();
                        }
                }
                else if (iter_v == m_visible_objects.cend())
                {
                        if (EXCLUSIVE_VISIBILITY)
                        {
                                m_visible_objects.clear();
                        }
                        m_visible_objects.insert(ptr);
                        m_visibility_changed();
                }
                return true;
        }

        const std::unordered_set<const T*>& visible_objects() const
        {
                return m_visible_objects;
        }
};

struct ViewportTransform
{
        // device_coordinates = (framebuffer_coordinates - center) * factor
        vec2 center;
        vec2 factor;
};
ViewportTransform viewport_transform(const Region<2, int>& viewport)
{
        const vec2 offset = to_vector<double>(viewport.from());
        const vec2 extent = to_vector<double>(viewport.extent());
        ViewportTransform t;
        t.center = offset + 0.5 * extent;
        t.factor = vec2(2.0 / extent[0], 2.0 / extent[1]);
        return t;
}

void clear_uint32_image_commands(const vulkan::ImageWithMemory& image, VkCommandBuffer command_buffer, uint32_t value)
{
        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.image();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        //

        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        //

        VkClearColorValue clear_color;

        ASSERT(image.format() == VK_FORMAT_R32_UINT);
        clear_color.uint32[0] = value;

        VkImageSubresourceRange range = barrier.subresourceRange;

        // Для vkCmdClearColorImage нужно VK_IMAGE_USAGE_TRANSFER_DST_BIT
        ASSERT((image.usage() & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        vkCmdClearColorImage(
                command_buffer, image.image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

        //

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);
}

class Impl final : public Renderer
{
        // Для получения текстуры для тени результат рисования находится в интервалах x(-1, 1) y(-1, 1) z(0, 1).
        // Для работы с этой текстурой надо преобразовать в интервалы x(0, 1) y(0, 1) z(0, 1).
        const mat4 SHADOW_TEXTURE_MATRIX = matrix::scale<double>(0.5, 0.5, 1) * matrix::translate<double>(1, 1, 0);

        const std::thread::id m_thread_id = std::this_thread::get_id();

        mat4 m_main_vp_matrix = mat4(1);
        mat4 m_shadow_vp_matrix = mat4(1);
        mat4 m_shadow_vp_texture_matrix = mat4(1);

        Color m_clear_color = Color(0);
        double m_shadow_zoom = 1;
        bool m_show_shadow = false;
        Region<2, int> m_viewport;
        std::optional<vec4> m_clip_plane;
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
        vulkan::Semaphore m_renderer_signal_semaphore;

        std::unique_ptr<DepthBuffers> m_mesh_renderer_depth_render_buffers;
        vulkan::Semaphore m_mesh_renderer_depth_signal_semaphore;
        MeshRenderer m_mesh_renderer;

        vulkan::Semaphore m_volume_renderer_signal_semaphore;
        VolumeRenderer m_volume_renderer;

        ObjectStorage<MeshObject> m_mesh_storage;
        ObjectStorage<VolumeObject> m_volume_storage;
        std::optional<ObjectId> m_current_object_id;

        std::optional<vulkan::CommandBuffers> m_default_command_buffers;

        void set_light_a(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_light_a(light);
        }
        void set_light_d(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_light_d(light);
        }
        void set_light_s(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_light_s(light);
        }
        void set_background_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clear_color = color;
                m_shader_buffers.set_background_color(color);

                create_command_buffers();
        }
        void set_default_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_default_color(color);
        }
        void set_default_specular_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_default_specular_color(color);
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
        void set_default_ns(double default_ns) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shader_buffers.set_default_ns(default_ns);
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
                create_mesh_render_command_buffers();
                create_mesh_depth_command_buffers();
        }
        void set_camera(const CameraInfo& c) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                const mat4& shadow_projection_matrix = matrix::ortho_vulkan<double>(
                        c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom, c.shadow_volume.top,
                        c.shadow_volume.near, c.shadow_volume.far);
                const mat4& main_projection_matrix = matrix::ortho_vulkan<double>(
                        c.main_volume.left, c.main_volume.right, c.main_volume.bottom, c.main_volume.top,
                        c.main_volume.near, c.main_volume.far);

                m_shadow_vp_matrix = shadow_projection_matrix * c.shadow_view_matrix;
                m_shadow_vp_texture_matrix = SHADOW_TEXTURE_MATRIX * m_shadow_vp_matrix;
                m_main_vp_matrix = main_projection_matrix * c.main_view_matrix;

                m_shader_buffers.set_direction_to_light(-to_vector<float>(c.light_direction));
                m_shader_buffers.set_direction_to_camera(-to_vector<float>(c.camera_direction));

                set_matrices();
        }

        void set_clip_plane(const std::optional<vec4>& plane) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clip_plane = plane;
                if (m_clip_plane)
                {
                        m_shader_buffers.set_clip_plane(*m_clip_plane, true);

                        VolumeObject* volume = m_volume_storage.find(m_current_object_id);
                        if (volume)
                        {
                                volume->set_clip_plane(*m_clip_plane);
                        }
                }
                else
                {
                        m_shader_buffers.set_clip_plane(vec4(0), false);
                }
                create_mesh_render_command_buffers();
        }

        void object_update(const mesh::MeshObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                mesh::ReadingUpdates reading(object);

                //

                ASSERT(m_volume_storage.find(object.id()) == nullptr);

                bool created = false;
                MeshObject* ptr = m_mesh_storage.find(object.id());
                if (!ptr)
                {
                        ptr = m_mesh_storage.insert(
                                object.id(),
                                create_mesh_object(
                                        m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                        m_transfer_queue, m_mesh_renderer.mesh_descriptor_sets_function(),
                                        m_mesh_renderer.material_descriptor_sets_function()));

                        created = true;
                }

                bool update_command_buffers;
                try
                {
                        if (!created)
                        {
                                ptr->update(reading.updates(), object, &update_command_buffers);
                        }
                        else
                        {
                                ptr->update({mesh::Update::All}, object, &update_command_buffers);
                        }
                }
                catch (const std::exception& e)
                {
                        m_mesh_storage.erase(object.id());
                        update_command_buffers = true;
                        created = false;
                        LOG(std::string("Error updating mesh object. ") + e.what());
                }
                catch (...)
                {
                        m_mesh_storage.erase(object.id());
                        update_command_buffers = true;
                        created = false;
                        LOG("Unknown error updating mesh object");
                }

                if ((created || update_command_buffers) && (m_current_object_id == object.id()))
                {
                        create_command_buffers();
                }
        }

        void object_update(const volume::VolumeObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                volume::ReadingUpdates reading(object);

                //

                ASSERT(m_mesh_storage.find(object.id()) == nullptr);

                bool created = false;
                VolumeObject* ptr = m_volume_storage.find(object.id());
                if (!ptr)
                {
                        ptr = m_volume_storage.insert(
                                object.id(),
                                create_volume_object(
                                        m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                        m_transfer_queue, m_volume_renderer.descriptor_sets_function()));

                        created = true;
                }

                bool update_command_buffers;
                try
                {
                        if (!created)
                        {
                                ptr->update(reading.updates(), object, &update_command_buffers);
                        }
                        else
                        {
                                ptr->update({volume::Update::All}, object, &update_command_buffers);
                        }
                }
                catch (const std::exception& e)
                {
                        m_volume_storage.erase(object.id());
                        update_command_buffers = true;
                        created = false;
                        LOG(std::string("Error updating volume object. ") + e.what());
                }
                catch (...)
                {
                        m_volume_storage.erase(object.id());
                        update_command_buffers = true;
                        created = false;
                        LOG("Unknown error updating volume object");
                }

                if ((created || update_command_buffers) && (m_current_object_id == object.id()))
                {
                        create_command_buffers();
                        if (created && m_current_object_id == object.id())
                        {
                                set_matrices();
                        }
                }
        }

        void object_delete(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const MeshObject* mesh = m_mesh_storage.find(id);
                const VolumeObject* volume = m_volume_storage.find(id);
                if (!mesh && !volume)
                {
                        return;
                }
                ASSERT(!mesh || !volume);

                bool delete_and_create_command_buffers = (m_current_object_id == id);
                if (delete_and_create_command_buffers)
                {
                        delete_command_buffers();
                }
                if (mesh)
                {
                        m_mesh_storage.erase(id);
                }
                if (volume)
                {
                        m_volume_storage.erase(id);
                }
                if (delete_and_create_command_buffers)
                {
                        create_command_buffers();
                }
        }

        void object_delete_all() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (m_mesh_storage.empty() && m_volume_storage.empty())
                {
                        return;
                }
                delete_command_buffers();
                m_mesh_storage.clear();
                m_volume_storage.clear();
                m_current_object_id.reset();
                create_command_buffers();
        }

        void object_show(ObjectId id, bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                std::optional<ObjectId> opt_id;
                if (show)
                {
                        opt_id = id;
                }
                if (m_current_object_id != opt_id)
                {
                        m_current_object_id = opt_id;
                        create_command_buffers();
                        set_matrices();
                }

                if (m_mesh_storage.set_visible(id, show))
                {
                        return;
                }
                if (m_volume_storage.set_visible(id, show))
                {
                        return;
                }
        }

        VkSemaphore draw(const vulkan::Queue& graphics_queue, unsigned image_index) const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(graphics_queue.family_index() == m_graphics_queue.family_index());

                ASSERT(image_index < m_swapchain->image_views().size());

                if (m_mesh_renderer.render_command_buffer(image_index))
                {
                        if (!m_show_shadow)
                        {
                                vulkan::queue_submit(
                                        *m_mesh_renderer.render_command_buffer(image_index),
                                        m_renderer_signal_semaphore, graphics_queue);
                        }
                        else
                        {
                                ASSERT(m_mesh_renderer.depth_command_buffer(image_index));
                                vulkan::queue_submit(
                                        *m_mesh_renderer.depth_command_buffer(image_index),
                                        m_mesh_renderer_depth_signal_semaphore, graphics_queue);

                                vulkan::queue_submit(
                                        m_mesh_renderer_depth_signal_semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        *m_mesh_renderer.render_command_buffer(image_index),
                                        m_renderer_signal_semaphore, graphics_queue);
                        }
                }
                else if (m_volume_renderer.command_buffer(image_index))
                {
                        vulkan::queue_submit(
                                *m_volume_renderer.command_buffer(image_index), m_renderer_signal_semaphore,
                                graphics_queue);
                }
                else
                {
                        ASSERT(m_default_command_buffers);
                        unsigned index = m_default_command_buffers->count() == 1 ? 0 : image_index;
                        vulkan::queue_submit(
                                (*m_default_command_buffers)[index], m_renderer_signal_semaphore, graphics_queue);
                }

                return m_renderer_signal_semaphore;
        }

        bool empty() const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                return m_default_command_buffers.has_value();
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

                m_mesh_renderer.create_render_buffers(m_render_buffers, m_object_image, m_viewport);
                create_mesh_depth_buffers();
                m_volume_renderer.create_buffers(m_render_buffers, m_viewport);

                create_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_volume_renderer.delete_buffers();
                delete_mesh_depth_buffers();
                m_mesh_renderer.delete_render_buffers();
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

        std::function<void(VkCommandBuffer)> before_render_pass_commands()
        {
                return [this](VkCommandBuffer command_buffer) {
                        uint32_t clear_value = 0;
                        clear_uint32_image_commands(*m_object_image, command_buffer, clear_value);
                };
        }

        void create_mesh_render_command_buffers()
        {
                m_mesh_renderer.delete_render_command_buffers();

                const MeshObject* mesh = m_mesh_storage.find(m_current_object_id);
                if (mesh)
                {
                        m_mesh_renderer.create_render_command_buffers(
                                {mesh}, m_graphics_command_pool, m_clip_plane.has_value(), m_show_normals,
                                m_clear_color, before_render_pass_commands());
                }
        }

        void create_mesh_depth_command_buffers()
        {
                m_mesh_renderer.delete_depth_command_buffers();

                const MeshObject* mesh = m_mesh_storage.find(m_current_object_id);
                if (mesh)
                {
                        m_mesh_renderer.create_depth_command_buffers(
                                {mesh}, m_graphics_command_pool, m_clip_plane.has_value(), m_show_normals);
                }
        }

        void create_volume_command_buffers()
        {
                m_volume_renderer.delete_command_buffers();

                const VolumeObject* volume = m_volume_storage.find(m_current_object_id);
                if (volume)
                {
                        m_volume_renderer.create_command_buffers(
                                volume, m_graphics_command_pool, m_clear_color, before_render_pass_commands());
                }
        }

        void create_default_command_buffers()
        {
                m_default_command_buffers.reset();

                if (m_mesh_storage.find(m_current_object_id) || m_volume_storage.find(m_current_object_id))
                {
                        return;
                }

                vulkan::CommandBufferCreateInfo info;

                info.device = m_device;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = m_render_buffers->width();
                info.render_area->extent.height = m_render_buffers->height();
                info.render_pass = m_render_buffers->render_pass();
                info.framebuffers = &m_render_buffers->framebuffers();
                info.command_pool = m_graphics_command_pool;
                const std::vector<VkClearValue> clear_values = m_render_buffers->clear_values(m_clear_color);
                info.clear_values = &clear_values;

                m_default_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_command_buffers()
        {
                create_mesh_render_command_buffers();
                create_mesh_depth_command_buffers();
                create_volume_command_buffers();
                create_default_command_buffers();
        }

        void delete_command_buffers()
        {
                m_default_command_buffers.reset();
                m_volume_renderer.delete_command_buffers();
                m_mesh_renderer.delete_render_command_buffers();
                m_mesh_renderer.delete_depth_command_buffers();
        }

        void set_matrices()
        {
                m_shader_buffers.set_matrices(m_main_vp_matrix, m_shadow_vp_matrix, m_shadow_vp_texture_matrix);

                VolumeObject* volume = m_volume_storage.find(m_current_object_id);
                if (volume)
                {
                        volume->set_matrix_and_clip_plane(m_main_vp_matrix, m_clip_plane);
                }
        }

        void mesh_visibility_changed()
        {
        }

        void volume_visibility_changed()
        {
                ASSERT(m_volume_storage.visible_objects().size() < 2);
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
                  m_renderer_signal_semaphore(m_device),
                  m_mesh_renderer_depth_signal_semaphore(m_device),
                  m_mesh_renderer(m_device, sample_shading, sampler_anisotropy, m_shader_buffers),
                  m_volume_renderer_signal_semaphore(m_device),
                  m_volume_renderer(m_device, sample_shading, m_shader_buffers),
                  m_mesh_storage([this]() { mesh_visibility_changed(); }),
                  m_volume_storage([this]() { volume_visibility_changed(); })
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
