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

template <typename T, typename Id>
std::enable_if_t<std::is_same_v<Id, ObjectId>, const T*> find_object(
        const std::unordered_map<ObjectId, std::unique_ptr<T>>& map,
        const Id& id)
{
        auto iter = map.find(id);
        return (iter != map.cend()) ? iter->second.get() : nullptr;
}
template <typename T, typename OptionalId>
std::enable_if_t<std::is_same_v<OptionalId, std::optional<ObjectId>>, const T*> find_object(
        const std::unordered_map<ObjectId, std::unique_ptr<T>>& map,
        const OptionalId& id)
{
        return id ? find_object(map, *id) : nullptr;
}

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

        std::unordered_map<ObjectId, std::unique_ptr<MeshObject>> m_mesh_storage;
        std::unordered_map<ObjectId, std::unique_ptr<VolumeObject>> m_volume_storage;
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

                        const VolumeObject* volume = find_object(m_volume_storage, m_current_object_id);
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

                ASSERT(find_object(m_volume_storage, object.id()) == nullptr);

                bool delete_and_create_command_buffers = (m_current_object_id == object.id());
                if (delete_and_create_command_buffers)
                {
                        delete_command_buffers();
                }

                m_mesh_storage.erase(object.id());
                m_mesh_storage.emplace(
                        object.id(),
                        std::make_unique<MeshObject>(
                                m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                m_transfer_queue, object, [this](const std::vector<MaterialInfo>& materials) {
                                        return m_mesh_renderer.create_material_descriptors_sets(materials);
                                }));

                if (delete_and_create_command_buffers)
                {
                        create_command_buffers();
                        set_matrices();
                }
        }
        void object_update(const volume::VolumeObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                volume::ReadingUpdates reading(object);

                //

                ASSERT(find_object(m_mesh_storage, object.id()) == nullptr);

                if (reading.updates().empty())
                {
                        return;
                }

                if (reading.updates().count(volume::Update::All) > 0)
                {
                        bool delete_and_create_command_buffers = (m_current_object_id == object.id());
                        if (delete_and_create_command_buffers)
                        {
                                delete_command_buffers();
                        }

                        m_volume_storage.erase(object.id());
                        m_volume_storage.emplace(
                                object.id(),
                                std::make_unique<VolumeObject>(
                                        m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                        m_transfer_queue, object, [this](const VolumeInfo& volume_info) {
                                                return m_volume_renderer.create_volume_memory(volume_info);
                                        }));

                        if (delete_and_create_command_buffers)
                        {
                                create_command_buffers();
                                set_matrices();
                        }
                }
                else
                {
                        error("Unsupported volume update type");
                }
        }
        void object_delete(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const MeshObject* mesh = find_object(m_mesh_storage, id);
                const VolumeObject* volume = find_object(m_volume_storage, id);
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
                        set_matrices();
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
                set_matrices();
        }
        void object_show(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (!(m_current_object_id == id))
                {
                        m_current_object_id = id;
                        create_command_buffers();
                        set_matrices();
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

        void create_mesh_render_command_buffers()
        {
                m_mesh_renderer.delete_render_command_buffers();

                const MeshObject* mesh = find_object(m_mesh_storage, m_current_object_id);
                if (mesh)
                {
                        m_mesh_renderer.create_render_command_buffers(
                                mesh, m_graphics_command_pool, m_clip_plane.has_value(), m_show_normals, m_clear_color,
                                [this](VkCommandBuffer command_buffer) {
                                        m_object_image->clear_commands(command_buffer, VK_IMAGE_LAYOUT_GENERAL);
                                });
                }
        }

        void create_mesh_depth_command_buffers()
        {
                m_mesh_renderer.delete_depth_command_buffers();

                const MeshObject* mesh = find_object(m_mesh_storage, m_current_object_id);
                if (mesh)
                {
                        m_mesh_renderer.create_depth_command_buffers(
                                mesh, m_graphics_command_pool, m_clip_plane.has_value(), m_show_normals);
                }
        }

        void create_volume_command_buffers()
        {
                m_volume_renderer.delete_command_buffers();

                const VolumeObject* volume = find_object(m_volume_storage, m_current_object_id);
                if (volume)
                {
                        m_volume_renderer.create_command_buffers(
                                volume, m_graphics_command_pool, m_clear_color, [this](VkCommandBuffer command_buffer) {
                                        m_object_image->clear_commands(command_buffer, VK_IMAGE_LAYOUT_GENERAL);
                                });
                }
        }

        void create_default_command_buffers()
        {
                m_default_command_buffers.reset();

                if (find_object(m_mesh_storage, m_current_object_id)
                    || find_object(m_volume_storage, m_current_object_id))
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
                const MeshObject* mesh = find_object(m_mesh_storage, m_current_object_id);
                if (mesh)
                {
                        const mat4& model = mesh->model_matrix();
                        const mat4& main_mvp = m_main_vp_matrix * model;
                        const mat4& shadow_mvp_texture = m_shadow_vp_texture_matrix * model;
                        const mat4& shadow_mvp = m_shadow_vp_matrix * model;

                        m_shader_buffers.set_matrices(
                                model, main_mvp, m_main_vp_matrix, shadow_mvp, m_shadow_vp_matrix, shadow_mvp_texture);
                }
                const VolumeObject* volume = find_object(m_volume_storage, m_current_object_id);
                if (volume)
                {
                        volume->set_coordinates(m_main_vp_matrix, m_clip_plane);
                }
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
                  m_volume_renderer(m_device, sample_shading, m_shader_buffers)
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
