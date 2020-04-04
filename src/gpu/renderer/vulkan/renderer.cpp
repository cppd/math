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
#include "shader_normals.h"
#include "shader_points.h"
#include "shader_shadow.h"
#include "shader_triangle_lines.h"
#include "shader_triangles.h"

#include "../com/storage.h"

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
#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
        vulkan::PhysicalDeviceFeatures::fragmentStoresAndAtomics,
        vulkan::PhysicalDeviceFeatures::geometryShader,
        vulkan::PhysicalDeviceFeatures::shaderClipDistance
};
// clang-format on

namespace gpu_vulkan
{
namespace
{
class Impl final : public Renderer
{
        // Для получения текстуры для тени результат рисования находится в интервалах x(-1, 1) y(-1, 1) z(0, 1).
        // Для работы с этой текстурой надо преобразовать в интервалы x(0, 1) y(0, 1) z(0, 1).
        const mat4 SHADOW_TEXTURE_MATRIX = scale<double>(0.5, 0.5, 1) * translate<double>(1, 1, 0);

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

        vulkan::Semaphore m_shadow_signal_semaphore;
        vulkan::Semaphore m_render_signal_semaphore;

        vulkan::Sampler m_texture_sampler;
        vulkan::Sampler m_shadow_sampler;

        RendererBuffers m_buffers;

        RendererTrianglesProgram m_triangles_program;
        RendererTrianglesSharedMemory m_triangles_shared_memory;

        RendererTriangleLinesProgram m_triangle_lines_program;
        RendererTriangleLinesMemory m_triangle_lines_memory;

        RendererNormalsProgram m_normals_program;
        RendererNormalsMemory m_normals_memory;

        RendererShadowProgram m_shadow_program;
        RendererShadowMemory m_shadow_memory;

        RendererPointsProgram m_points_program;
        RendererPointsMemory m_points_memory;

        RenderBuffers3D* m_render_buffers = nullptr;
        std::optional<vulkan::Pipeline> m_render_triangles_pipeline;
        std::optional<vulkan::Pipeline> m_render_triangle_lines_pipeline;
        std::optional<vulkan::Pipeline> m_render_normals_pipeline;
        std::optional<vulkan::Pipeline> m_render_points_pipeline;
        std::optional<vulkan::Pipeline> m_render_lines_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_command_buffers;

        std::unique_ptr<RendererDepthBuffers> m_shadow_buffers;
        std::optional<vulkan::Pipeline> m_shadow_pipeline;
        std::optional<vulkan::CommandBuffers> m_shadow_command_buffers;

        const vulkan::ImageWithMemory* m_object_image = nullptr;

        RendererObjectStorage<ObjectId, MeshObject> m_storage;

        unsigned m_x, m_y, m_width, m_height;

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

                create_shadow_buffers();
                create_all_command_buffers();
        }
        void set_camera(const RendererCameraInfo& c) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                const mat4& shadow_projection_matrix = ortho_vulkan<double>(
                        c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom, c.shadow_volume.top,
                        c.shadow_volume.near, c.shadow_volume.far);
                const mat4& main_projection_matrix = ortho_vulkan<double>(
                        c.main_volume.left, c.main_volume.right, c.main_volume.bottom, c.main_volume.top,
                        c.main_volume.near, c.main_volume.far);

                m_shadow_vp_matrix = shadow_projection_matrix * c.shadow_view_matrix;
                m_shadow_vp_texture_matrix = SHADOW_TEXTURE_MATRIX * m_shadow_vp_matrix;
                m_main_vp_matrix = main_projection_matrix * c.main_view_matrix;

                m_buffers.set_direction_to_light(-to_vector<float>(c.light_direction));
                m_buffers.set_direction_to_camera(-to_vector<float>(c.camera_direction));

                set_matrices();
        }

        void set_clip_plane() const
        {
                if (m_clip_plane)
                {
                        vec4 main_plane = *m_clip_plane * m_main_vp_matrix.inverse();
                        vec4 shadow_plane = *m_clip_plane * m_shadow_vp_matrix.inverse();
                        m_buffers.set_clip_plane(main_plane, shadow_plane, true);
                }
        }

        void clip_plane_show(const vec4& plane) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clip_plane = plane;

                set_clip_plane();

                create_render_command_buffers();
        }

        void clip_plane_hide() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                m_clip_plane.reset();

                m_buffers.set_clip_plane(vec4(0), vec4(0), false);

                create_render_command_buffers();
        }

        void object_add(const mesh::MeshObject<3>& object) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                std::unique_ptr draw_object = std::make_unique<MeshObject>(
                        m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool, m_transfer_queue,
                        m_texture_sampler, m_triangles_program.descriptor_set_layout_material(), object.mesh(),
                        object.matrix());

                bool delete_and_create_command_buffers = m_storage.is_current_object(object.id());
                if (delete_and_create_command_buffers)
                {
                        delete_all_command_buffers();
                        m_storage.delete_object(object.id());
                }
                m_storage.add_object(std::move(draw_object), object.id());
                if (delete_and_create_command_buffers)
                {
                        m_storage.show_object(object.id());
                        create_all_command_buffers();
                }

                set_matrices();
        }
        void object_delete(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                bool delete_and_create_command_buffers = m_storage.is_current_object(id);
                if (delete_and_create_command_buffers)
                {
                        delete_all_command_buffers();
                }
                m_storage.delete_object(id);
                if (delete_and_create_command_buffers)
                {
                        create_all_command_buffers();
                }
                set_matrices();
        }
        void object_delete_all() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                bool delete_and_create_command_buffers = m_storage.object() != nullptr;
                if (delete_and_create_command_buffers)
                {
                        delete_all_command_buffers();
                }
                m_storage.delete_all();
                if (delete_and_create_command_buffers)
                {
                        create_all_command_buffers();
                }
                set_matrices();
        }
        void object_show(ObjectId id) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_storage.is_current_object(id))
                {
                        return;
                }
                const MeshObject* object = m_storage.object();
                m_storage.show_object(id);
                if (object != m_storage.object())
                {
                        create_all_command_buffers();
                }
                set_matrices();
        }

        VkSemaphore draw(const vulkan::Queue& graphics_queue, unsigned image_index) const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(graphics_queue.family_index() == m_graphics_queue.family_index());

                ASSERT(image_index < m_swapchain->image_views().size());
                ASSERT(m_render_command_buffers->count() == m_swapchain->image_views().size() ||
                       m_render_command_buffers->count() == 1);

                const unsigned render_index = m_render_command_buffers->count() == 1 ? 0 : image_index;

                if (!m_show_shadow || !m_storage.object() || !m_storage.object()->has_shadow())
                {
                        vulkan::queue_submit(
                                (*m_render_command_buffers)[render_index], m_render_signal_semaphore, graphics_queue);
                }
                else
                {
                        ASSERT(m_shadow_command_buffers->count() == m_swapchain->image_views().size() ||
                               m_shadow_command_buffers->count() == 1);

                        const unsigned shadow_index = m_shadow_command_buffers->count() == 1 ? 0 : image_index;

                        vulkan::queue_submit(
                                (*m_shadow_command_buffers)[shadow_index], m_shadow_signal_semaphore, graphics_queue);

                        //

                        vulkan::queue_submit(
                                m_shadow_signal_semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                (*m_render_command_buffers)[render_index], m_render_signal_semaphore, graphics_queue);
                }

                return m_render_signal_semaphore;
        }

        bool empty() const override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                return m_storage.object() == nullptr;
        }

        void create_buffers(
                const vulkan::Swapchain* swapchain,
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(x + width <= objects->width() && y + height <= objects->height());

                m_swapchain = swapchain;
                m_render_buffers = render_buffers;
                m_object_image = objects;
                m_x = x;
                m_y = y;
                m_width = width;
                m_height = height;

                create_render_buffers();
                create_shadow_buffers();

                create_all_command_buffers();
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                delete_shadow_buffers();
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

                m_triangles_shared_memory.set_object_image(m_object_image);
                m_points_memory.set_object_image(m_object_image);

                m_render_triangles_pipeline = m_triangles_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading, m_x, m_y,
                        m_width, m_height);
                m_render_triangle_lines_pipeline = m_triangle_lines_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading, m_x, m_y,
                        m_width, m_height);
                m_render_normals_pipeline = m_normals_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(), m_sample_shading, m_x, m_y,
                        m_width, m_height);
                m_render_points_pipeline = m_points_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(),
                        VK_PRIMITIVE_TOPOLOGY_POINT_LIST, m_x, m_y, m_width, m_height);
                m_render_lines_pipeline = m_points_program.create_pipeline(
                        m_render_buffers->render_pass(), m_render_buffers->sample_count(),
                        VK_PRIMITIVE_TOPOLOGY_LINE_LIST, m_x, m_y, m_width, m_height);
        }

        void delete_shadow_buffers()
        {
                m_shadow_command_buffers.reset();
                m_shadow_pipeline.reset();
                m_shadow_buffers.reset();
        }

        void create_shadow_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_swapchain);

                delete_shadow_buffers();

                //

                constexpr RendererDepthBufferCount buffer_count = RendererDepthBufferCount::One;
                m_shadow_buffers = create_renderer_depth_buffers(
                        buffer_count, *m_swapchain, {m_graphics_queue.family_index()}, m_graphics_command_pool,
                        m_graphics_queue, m_device, m_width, m_height, m_shadow_zoom);

                m_triangles_shared_memory.set_shadow_texture(m_shadow_sampler, m_shadow_buffers->texture(0));

                m_shadow_pipeline = m_shadow_program.create_pipeline(
                        m_shadow_buffers->render_pass(), m_shadow_buffers->sample_count(), 0, 0,
                        m_shadow_buffers->width(), m_shadow_buffers->height());
        }

        void set_matrices()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (m_storage.object())
                {
                        const mat4& model = m_storage.object()->model_matrix();
                        const mat4& main_mvp = m_main_vp_matrix * model;
                        const mat4& shadow_mvp_texture = m_shadow_vp_texture_matrix * model;
                        const mat4& shadow_mvp = m_shadow_vp_matrix * model;

                        m_buffers.set_matrices(main_mvp, model, m_main_vp_matrix, shadow_mvp, shadow_mvp_texture);

                        set_clip_plane();
                }
        }

        void before_render_pass_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_object_image->clear_commands(command_buffer, VK_IMAGE_LAYOUT_GENERAL);
        }

        void draw_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (!m_storage.object())
                {
                        return;
                }

                {
                        MeshObject::DrawInfoAll info;

                        info.triangles.pipeline_layout = m_triangles_program.pipeline_layout();
                        info.triangles.pipeline = *m_render_triangles_pipeline;
                        info.triangles.shared_descriptor_set = m_triangles_shared_memory.descriptor_set();
                        info.triangles.shared_descriptor_set_number = RendererTrianglesSharedMemory::set_number();

                        info.lines.pipeline_layout = m_points_program.pipeline_layout();
                        info.lines.pipeline = *m_render_lines_pipeline;
                        info.lines.descriptor_set = m_points_memory.descriptor_set();
                        info.lines.descriptor_set_number = RendererPointsMemory::set_number();

                        info.points.pipeline_layout = m_points_program.pipeline_layout();
                        info.points.pipeline = *m_render_points_pipeline;
                        info.points.descriptor_set = m_points_memory.descriptor_set();
                        info.points.descriptor_set_number = RendererPointsMemory::set_number();

                        m_storage.object()->draw_commands_all(command_buffer, info);
                }

                if (m_clip_plane)
                {
                        MeshObject::DrawInfoPlainTriangles info;

                        info.pipeline_layout = m_triangle_lines_program.pipeline_layout();
                        info.pipeline = *m_render_triangle_lines_pipeline;
                        info.descriptor_set = m_triangle_lines_memory.descriptor_set();
                        info.descriptor_set_number = RendererTriangleLinesMemory::set_number();

                        m_storage.object()->draw_commands_plain_triangles(command_buffer, info);
                }

                if (m_show_normals)
                {
                        MeshObject::DrawInfoTriangleVertices info;

                        info.pipeline_layout = m_normals_program.pipeline_layout();
                        info.pipeline = *m_render_normals_pipeline;
                        info.descriptor_set = m_normals_memory.descriptor_set();
                        info.descriptor_set_number = RendererNormalsMemory::set_number();

                        m_storage.object()->draw_commands_triangle_vertices(command_buffer, info);
                }
        }

        void draw_shadow_commands(VkCommandBuffer command_buffer) const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                if (!m_storage.object())
                {
                        return;
                }

                MeshObject::DrawInfoPlainTriangles info;

                info.pipeline_layout = m_shadow_program.pipeline_layout();
                info.pipeline = *m_shadow_pipeline;
                info.descriptor_set = m_shadow_memory.descriptor_set();
                info.descriptor_set_number = RendererShadowMemory::set_number();

                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);

                m_storage.object()->draw_commands_plain_triangles(command_buffer, info);
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
                info.before_render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        before_render_pass_commands(command_buffer);
                };
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) { draw_commands(command_buffer); };
                const std::vector<VkClearValue> clear_values = m_render_buffers->clear_values(m_clear_color);
                info.clear_values = &clear_values;
                m_render_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_shadow_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_shadow_buffers);

                m_shadow_command_buffers.reset();

                vulkan::CommandBufferCreateInfo info;
                info.device = m_device;
                info.render_area.emplace();
                info.render_area->offset.x = 0;
                info.render_area->offset.y = 0;
                info.render_area->extent.width = m_shadow_buffers->width();
                info.render_area->extent.height = m_shadow_buffers->height();
                info.render_pass = m_shadow_buffers->render_pass();
                info.framebuffers = &m_shadow_buffers->framebuffers();
                info.command_pool = m_graphics_command_pool;
                info.clear_values = &m_shadow_buffers->clear_values();
                info.render_pass_commands = [this](VkCommandBuffer command_buffer) {
                        draw_shadow_commands(command_buffer);
                };
                m_shadow_command_buffers = vulkan::create_command_buffers(info);
        }

        void create_all_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                create_render_command_buffers();
                create_shadow_command_buffers();
        }

        void delete_all_command_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_render_buffers);

                m_render_command_buffers.reset();
                m_shadow_command_buffers.reset();
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
                  m_shadow_signal_semaphore(m_device),
                  m_render_signal_semaphore(m_device),
                  m_texture_sampler(create_renderer_texture_sampler(m_device, sampler_anisotropy)),
                  m_shadow_sampler(create_renderer_shadow_sampler(m_device)),
                  //
                  m_buffers(m_device, {m_graphics_queue.family_index()}),
                  //
                  m_triangles_program(m_device),
                  m_triangles_shared_memory(m_device, m_triangles_program.descriptor_set_layout_shared(), m_buffers),
                  //
                  m_triangle_lines_program(m_device),
                  m_triangle_lines_memory(m_device, m_triangle_lines_program.descriptor_set_layout(), m_buffers),
                  //
                  m_normals_program(m_device),
                  m_normals_memory(m_device, m_normals_program.descriptor_set_layout(), m_buffers),
                  //
                  m_shadow_program(m_device),
                  m_shadow_memory(m_device, m_shadow_program.descriptor_set_layout(), m_buffers),
                  //
                  m_points_program(m_device),
                  m_points_memory(m_device, m_points_program.descriptor_set_layout(), m_buffers)
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
