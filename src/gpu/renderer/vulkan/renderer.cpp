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
#include "sampler.h"

#include "shader/normals.h"
#include "shader/points.h"
#include "shader/triangle_lines.h"
#include "shader/triangles.h"
#include "shader/triangles_depth.h"

#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/time.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>
#include <src/utility/string/vector.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/create.h>
#include <src/vulkan/device.h>
#include <src/vulkan/error.h>
#include <src/vulkan/query.h>
#include <src/vulkan/queue.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>

namespace gpu
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

class Impl final : public Renderer
{
        // Для получения текстуры для тени результат рисования находится в интервалах x(-1, 1) y(-1, 1) z(0, 1).
        // Для работы с этой текстурой надо преобразовать в интервалы x(0, 1) y(0, 1) z(0, 1).
        const mat4 SHADOW_TEXTURE_MATRIX = matrix::scale<double>(0.5, 0.5, 1) * matrix::translate<double>(1, 1, 0);

        const std::thread::id m_thread_id = std::this_thread::get_id();

        const bool m_sample_shading;

        Color m_clear_color = Color(0);

        mat4 m_main_vp_matrix = mat4(1);
        mat4 m_shadow_vp_matrix = mat4(1);
        mat4 m_shadow_vp_texture_matrix = mat4(1);

        double m_shadow_zoom = 1;
        bool m_show_shadow = false;

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;
        const vulkan::CommandPool& m_transfer_command_pool;
        const vulkan::Queue& m_transfer_queue;

        const vulkan::Swapchain* m_swapchain = nullptr;

        vulkan::Semaphore m_render_signal_semaphore;
        vulkan::Semaphore m_render_depth_signal_semaphore;

        vulkan::Sampler m_texture_sampler;
        vulkan::Sampler m_shadow_sampler;

        RendererBuffers m_buffers;

        RendererTrianglesProgram m_triangles_program;
        RendererTrianglesMemory m_triangles_memory;

        RendererTriangleLinesProgram m_triangle_lines_program;
        RendererTriangleLinesMemory m_triangle_lines_memory;

        RendererNormalsProgram m_normals_program;
        RendererNormalsMemory m_normals_memory;

        RendererTrianglesDepthProgram m_triangles_depth_program;
        RendererTrianglesDepthMemory m_triangles_depth_memory;

        RendererPointsProgram m_points_program;
        RendererPointsMemory m_points_memory;

        RenderBuffers3D* m_render_buffers = nullptr;
        std::optional<vulkan::Pipeline> m_render_triangles_pipeline;
        std::optional<vulkan::Pipeline> m_render_triangle_lines_pipeline;
        std::optional<vulkan::Pipeline> m_render_normals_pipeline;
        std::optional<vulkan::Pipeline> m_render_points_pipeline;
        std::optional<vulkan::Pipeline> m_render_lines_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_command_buffers;

        std::unique_ptr<RendererDepthBuffers> m_depth_buffers;
        std::optional<vulkan::Pipeline> m_render_triangles_depth_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_depth_command_buffers;

        const vulkan::ImageWithMemory* m_object_image = nullptr;

        std::unordered_map<ObjectId, std::unique_ptr<MeshObject>> m_mesh_storage;
        std::optional<ObjectId> m_current_object_id;

        Region<2, int> m_viewport;

        std::optional<vec4> m_clip_plane;
        bool m_show_normals = false;

        void set_light_a(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_light_a(light);
        }
        void set_light_d(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_light_d(light);
        }
        void set_light_s(const Color& light) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_light_s(light);
        }
        void set_background_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clear_color = color;
                m_buffers.set_background_color(color);

                create_render_command_buffers();
        }
        void set_default_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_default_color(color);
        }
        void set_wireframe_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_wireframe_color(color);
        }
        void set_clip_plane_color(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_clip_plane_color(color);
        }
        void set_normal_length(float length) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_normal_length(length);
        }
        void set_normal_color_positive(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_normal_color_positive(color);
        }
        void set_normal_color_negative(const Color& color) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_normal_color_negative(color);
        }
        void set_default_ns(double default_ns) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_default_ns(default_ns);
        }
        void set_show_smooth(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_show_smooth(show);
        }
        void set_show_wireframe(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_show_wireframe(show);
        }
        void set_show_shadow(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_show_shadow(show);
                m_show_shadow = show;
        }
        void set_show_fog(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_show_fog(show);
        }
        void set_show_materials(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_buffers.set_show_materials(show);
        }
        void set_show_normals(bool show) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_show_normals != show)
                {
                        m_show_normals = show;
                        create_render_command_buffers();
                }
        }
        void set_shadow_zoom(double zoom) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_shadow_zoom = zoom;

                create_depth_buffers();
                create_all_command_buffers();
        }
        void set_camera(const RendererCameraInfo& c) override
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

                m_buffers.set_direction_to_light(-to_vector<float>(c.light_direction));
                m_buffers.set_direction_to_camera(-to_vector<float>(c.camera_direction));

                set_matrices();
        }

        void set_clip_plane(const std::optional<vec4>& plane) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clip_plane = plane;
                if (m_clip_plane)
                {
                        m_buffers.set_clip_plane(*m_clip_plane, true);
                }
                else
                {
                        m_buffers.set_clip_plane(vec4(0), false);
                }
                create_render_command_buffers();
        }

        void object_add(const mesh::MeshObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                std::unique_ptr draw_object = std::make_unique<MeshObject>(
                        m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool, m_transfer_queue,
                        object.mesh(), object.matrix());

                draw_object->create_descriptor_sets([&](const std::vector<MaterialInfo>& materials) {
                        return RendererTrianglesMaterialMemory::create(
                                m_device, m_texture_sampler, m_triangles_program.descriptor_set_layout_material(),
                                materials);
                });

                bool delete_and_create_command_buffers = (m_current_object_id == object.id());
                if (delete_and_create_command_buffers)
                {
                        delete_all_command_buffers();
                }
                m_mesh_storage.insert_or_assign(object.id(), std::move(draw_object));
                if (delete_and_create_command_buffers)
                {
                        create_all_command_buffers();
                        set_matrices();
                }
        }
        void object_add(const volume::VolumeObject<3>& /*object*/) override
        {
        }
        void object_delete(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (find_object(m_mesh_storage, id) == nullptr)
                {
                        return;
                }
                bool delete_and_create_command_buffers = (m_current_object_id == id);
                if (delete_and_create_command_buffers)
                {
                        delete_all_command_buffers();
                }
                m_mesh_storage.erase(id);
                if (delete_and_create_command_buffers)
                {
                        create_all_command_buffers();
                        set_matrices();
                }
        }
        void object_delete_all() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_mesh_storage.empty())
                {
                        return;
                }
                delete_all_command_buffers();
                m_mesh_storage.clear();
                m_current_object_id.reset();
                create_all_command_buffers();
                set_matrices();
        }
        void object_show(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (!(m_current_object_id == id))
                {
                        m_current_object_id = id;
                        create_all_command_buffers();
                        set_matrices();
                }
        }

        VkSemaphore draw(const vulkan::Queue& graphics_queue, unsigned image_index) const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(graphics_queue.family_index() == m_graphics_queue.family_index());

                ASSERT(image_index < m_swapchain->image_views().size());
                ASSERT(m_render_command_buffers->count() == m_swapchain->image_views().size()
                       || m_render_command_buffers->count() == 1);

                const unsigned render_index = m_render_command_buffers->count() == 1 ? 0 : image_index;

                if (!m_show_shadow)
                {
                        vulkan::queue_submit(
                                (*m_render_command_buffers)[render_index], m_render_signal_semaphore, graphics_queue);
                }
                else
                {
                        ASSERT(m_render_depth_command_buffers->count() == m_swapchain->image_views().size()
                               || m_render_depth_command_buffers->count() == 1);

                        const unsigned render_depth_index =
                                m_render_depth_command_buffers->count() == 1 ? 0 : image_index;

                        vulkan::queue_submit(
                                (*m_render_depth_command_buffers)[render_depth_index], m_render_depth_signal_semaphore,
                                graphics_queue);

                        //

                        vulkan::queue_submit(
                                m_render_depth_signal_semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                (*m_render_command_buffers)[render_index], m_render_signal_semaphore, graphics_queue);
                }

                return m_render_signal_semaphore;
        }

        bool empty() const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                return find_object(m_mesh_storage, m_current_object_id) == nullptr;
        }

        void create_buffers(
                const vulkan::Swapchain* swapchain,
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                const Region<2, int>& viewport) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(viewport.x1() <= static_cast<int>(objects->width()));
                ASSERT(viewport.y1() <= static_cast<int>(objects->height()));

                m_swapchain = swapchain;
                m_render_buffers = render_buffers;
                m_object_image = objects;
                m_viewport = viewport;

                create_render_buffers();
                create_depth_buffers();

                create_all_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                delete_depth_buffers();
                delete_render_buffers();
        }

        void delete_render_buffers()
        {
                m_render_command_buffers.reset();
                m_render_triangles_pipeline.reset();
                m_render_triangle_lines_pipeline.reset();
                m_render_normals_pipeline.reset();
                m_render_points_pipeline.reset();
                m_render_lines_pipeline.reset();
        }

        void create_render_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_swapchain);
                ASSERT(m_render_buffers);
                ASSERT(m_object_image);

                delete_render_buffers();

                //

                m_triangles_memory.set_object_image(m_object_image);
                m_points_memory.set_object_image(m_object_image);

                m_render_triangles_pipeline = m_triangles_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading,
                        m_viewport);
                m_render_triangle_lines_pipeline = m_triangle_lines_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading,
                        m_viewport);
                m_render_normals_pipeline = m_normals_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading,
                        m_viewport);
                m_render_points_pipeline = m_points_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(),
                        VK_PRIMITIVE_TOPOLOGY_POINT_LIST, m_viewport);
                m_render_lines_pipeline = m_points_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(),
                        VK_PRIMITIVE_TOPOLOGY_LINE_LIST, m_viewport);
        }

        void delete_depth_buffers()
        {
                m_render_depth_command_buffers.reset();
                m_render_triangles_depth_pipeline.reset();
                m_depth_buffers.reset();
        }

        void create_depth_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_swapchain);

                delete_depth_buffers();

                //

                constexpr RendererDepthBufferCount buffer_count = RendererDepthBufferCount::One;
                m_depth_buffers = create_renderer_depth_buffers(
                        buffer_count, *m_swapchain, {m_graphics_queue.family_index()}, m_graphics_command_pool,
                        m_graphics_queue, m_device, m_viewport.width(), m_viewport.height(), m_shadow_zoom);

                m_triangles_memory.set_shadow_texture(m_shadow_sampler, m_depth_buffers->texture(0));

                m_render_triangles_depth_pipeline = m_triangles_depth_program.create_pipeline(
                        m_depth_buffers->render_pass(), m_depth_buffers->sample_count(),
                        Region<2, int>(0, 0, m_depth_buffers->width(), m_depth_buffers->height()));
        }

        void set_matrices()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const MeshObject* mesh = find_object(m_mesh_storage, m_current_object_id);
                if (!mesh)
                {
                        return;
                }

                const mat4& model = mesh->model_matrix();
                const mat4& main_mvp = m_main_vp_matrix * model;
                const mat4& shadow_mvp_texture = m_shadow_vp_texture_matrix * model;
                const mat4& shadow_mvp = m_shadow_vp_matrix * model;

                m_buffers.set_matrices(
                        model, main_mvp, m_main_vp_matrix, shadow_mvp, m_shadow_vp_matrix, shadow_mvp_texture);
        }

        void before_render_pass_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_object_image->clear_commands(command_buffer, VK_IMAGE_LAYOUT_GENERAL);
        }

        void draw_commands(VkCommandBuffer command_buffer, bool depth) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                const MeshObject* mesh = find_object(m_mesh_storage, m_current_object_id);

                if (!mesh)
                {
                        return;
                }

                if (depth)
                {
                        vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);
                }

                if (!depth)
                {
                        vkCmdBindPipeline(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangles_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                                RendererTrianglesMemory::set_number(), 1 /*set count*/,
                                &m_triangles_memory.descriptor_set(), 0, nullptr);

                        auto bind_material_descriptor_set = [&](VkDescriptorSet descriptor_set) {
                                vkCmdBindDescriptorSets(
                                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        m_triangles_program.pipeline_layout(),
                                        RendererTrianglesMaterialMemory::set_number(), 1 /*set count*/, &descriptor_set,
                                        0, nullptr);
                        };

                        mesh->commands_triangles(
                                command_buffer, m_triangles_program.descriptor_set_layout_material(),
                                bind_material_descriptor_set);
                }
                else
                {
                        vkCmdBindPipeline(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangles_depth_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_triangles_depth_program.pipeline_layout(), RendererTrianglesDepthMemory::set_number(),
                                1 /*set count*/, &m_triangles_depth_memory.descriptor_set(), 0, nullptr);

                        mesh->commands_plain_triangles(command_buffer);
                }

                if (!depth)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_lines_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                                RendererPointsMemory::set_number(), 1 /*set count*/, &m_points_memory.descriptor_set(),
                                0, nullptr);

                        mesh->commands_lines(command_buffer);
                }

                if (!depth)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_points_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                                RendererPointsMemory::set_number(), 1 /*set count*/, &m_points_memory.descriptor_set(),
                                0, nullptr);

                        mesh->commands_points(command_buffer);
                }

                if (!depth && m_clip_plane)
                {
                        vkCmdBindPipeline(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangle_lines_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_triangle_lines_program.pipeline_layout(), RendererTriangleLinesMemory::set_number(),
                                1 /*set count*/, &m_triangle_lines_memory.descriptor_set(), 0, nullptr);

                        mesh->commands_plain_triangles(command_buffer);
                }

                if (!depth && m_show_normals)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_normals_pipeline);

                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normals_program.pipeline_layout(),
                                RendererNormalsMemory::set_number(), 1 /*set count*/,
                                &m_normals_memory.descriptor_set(), 0, nullptr);

                        mesh->commands_triangle_vertices(command_buffer);
                }
        }

        void create_render_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_render_buffers);

                m_render_command_buffers.reset();

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
                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        before_render_pass_commands(command_buffer);
                };
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        draw_commands(command_buffer, false /*depth*/);
                };

                m_render_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_render_depth_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_depth_buffers);

                m_render_depth_command_buffers.reset();

                vulkan::CommandBufferCreateInfo info;

                info.device = m_device;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = m_depth_buffers->width();
                info.render_area->extent.height = m_depth_buffers->height();
                info.render_pass = m_depth_buffers->render_pass();
                info.framebuffers = &m_depth_buffers->framebuffers();
                info.command_pool = m_graphics_command_pool;
                info.clear_values = &m_depth_buffers->clear_values();
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        draw_commands(command_buffer, true /*depth*/);
                };

                m_render_depth_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_all_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                create_render_command_buffers();
                create_render_depth_command_buffers();
        }

        void delete_all_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_render_buffers);

                m_render_command_buffers.reset();
                m_render_depth_command_buffers.reset();
        }

public:
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             bool sample_shading,
             bool sampler_anisotropy)
                : m_sample_shading(sample_shading),
                  m_instance(instance),
                  m_device(instance.device()),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_render_signal_semaphore(m_device),
                  m_render_depth_signal_semaphore(m_device),
                  m_texture_sampler(create_renderer_texture_sampler(m_device, sampler_anisotropy)),
                  m_shadow_sampler(create_renderer_shadow_sampler(m_device)),
                  //
                  m_buffers(m_device, {m_graphics_queue.family_index()}),
                  //
                  m_triangles_program(m_device),
                  m_triangles_memory(
                          m_device,
                          m_triangles_program.descriptor_set_layout(),
                          m_buffers.matrices_buffer(),
                          m_buffers.lighting_buffer(),
                          m_buffers.drawing_buffer()),
                  //
                  m_triangle_lines_program(m_device),
                  m_triangle_lines_memory(
                          m_device,
                          m_triangle_lines_program.descriptor_set_layout(),
                          m_buffers.matrices_buffer(),
                          m_buffers.drawing_buffer()),
                  //
                  m_normals_program(m_device),
                  m_normals_memory(
                          m_device,
                          m_normals_program.descriptor_set_layout(),
                          m_buffers.matrices_buffer(),
                          m_buffers.drawing_buffer()),
                  //
                  m_triangles_depth_program(m_device),
                  m_triangles_depth_memory(
                          m_device,
                          m_triangles_depth_program.descriptor_set_layout(),
                          m_buffers.shadow_matrices_buffer(),
                          m_buffers.drawing_buffer()),
                  //
                  m_points_program(m_device),
                  m_points_memory(
                          m_device,
                          m_points_program.descriptor_set_layout(),
                          m_buffers.matrices_buffer(),
                          m_buffers.drawing_buffer())

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
