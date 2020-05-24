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

#include "view.h"

#include "render_buffer.h"
#include "resolve.h"

#include "../com/camera.h"
#include "../com/frame_rate.h"
#include "../com/view_thread.h"
#include "../com/window.h"

#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/merge.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/variant.h>
#include <src/gpu/convex_hull/vulkan/view.h>
#include <src/gpu/dft/vulkan/view.h>
#include <src/gpu/optical_flow/vulkan/view.h>
#include <src/gpu/pencil_sketch/vulkan/view.h>
#include <src/gpu/renderer/vulkan/renderer.h>
#include <src/gpu/text_writer/vulkan/view.h>
#include <src/numerical/region.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>
#include <src/window/manage.h>

namespace view
{
namespace
{
constexpr double IDLE_MODE_FRAME_DURATION_IN_SECONDS = 0.1;

// 2 - double buffering, 3 - triple buffering
constexpr int VULKAN_PREFERRED_IMAGE_COUNT = 2;
// Шейдеры пишут результат в цветовом пространстве RGB, поэтому _SRGB (для результата в sRGB нужен _UNORM).
constexpr VkSurfaceFormatKHR VULKAN_SURFACE_FORMAT = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

constexpr int VULKAN_MINIMUM_SAMPLE_COUNT = 4;

// Supersampling
constexpr bool VULKAN_SAMPLE_SHADING = true;
// Anisotropic filtering
constexpr bool VULKAN_SAMPLER_ANISOTROPY = true;

// Это только для начального значения, а далее оно устанавливается командой set_vertical_sync
constexpr vulkan::PresentMode VULKAN_DEFAULT_PRESENT_MODE = vulkan::PresentMode::PreferFast;

constexpr VkFormat VULKAN_OBJECT_IMAGE_FORMAT = VK_FORMAT_R32_UINT;

//

constexpr double FRAME_SIZE_IN_MILLIMETERS = 0.5;

//

std::vector<vulkan::PhysicalDeviceFeatures> device_features_sample_shading(int sample_count, bool sample_shading)
{
        if (sample_count > 1 && sample_shading)
        {
                return {vulkan::PhysicalDeviceFeatures::sampleRateShading};
        }

        return {};
}

std::vector<vulkan::PhysicalDeviceFeatures> device_features_sampler_anisotropy(bool sampler_anisotropy)
{
        if (sampler_anisotropy)
        {
                return {vulkan::PhysicalDeviceFeatures::samplerAnisotropy};
        }

        return {};
}

class Impl final : public View
{
        const double m_window_ppi;
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const int m_frame_size_in_pixels = std::max(1, millimeters_to_pixels(FRAME_SIZE_IN_MILLIMETERS, m_window_ppi));

        FrameRate m_frame_rate{m_window_ppi};
        Camera m_camera;

        Region<2, int> m_draw_rectangle{limits<int>::lowest(), limits<int>::lowest(), 0, 0};

        //

        vulkan::PresentMode m_present_mode = VULKAN_DEFAULT_PRESENT_MODE;

        bool m_text_active = true;
        bool m_convex_hull_active = false;
        bool m_pencil_sketch_active = false;
        bool m_dft_active = false;
        bool m_optical_flow_active = false;

        // В последовательности swapchain, а затем renderer,
        // так как буферы renderer могут зависеть от swapchain
        std::unique_ptr<vulkan::VulkanInstance> m_instance;
        std::unique_ptr<vulkan::Semaphore> m_image_semaphore;
        std::unique_ptr<vulkan::Swapchain> m_swapchain;
        std::unique_ptr<RenderBuffers> m_render_buffers;
        std::unique_ptr<vulkan::ImageWithMemory> m_resolve_texture;
        std::unique_ptr<vulkan::CommandBuffers> m_resolve_command_buffers;
        std::unique_ptr<vulkan::Semaphore> m_resolve_semaphore;
        std::unique_ptr<vulkan::ImageWithMemory> m_object_image;
        std::unique_ptr<gpu::renderer::Renderer> m_renderer;
        std::unique_ptr<gpu::text_writer::View> m_text;
        std::unique_ptr<gpu::convex_hull::View> m_convex_hull;
        std::unique_ptr<gpu::pencil_sketch::View> m_pencil_sketch;
        std::unique_ptr<gpu::dft::View> m_dft;
        std::unique_ptr<gpu::optical_flow::View> m_optical_flow;

        //

        std::optional<mat4> m_clip_plane_view_matrix;

        //

        struct PressedMouseButton
        {
                bool pressed = false;
                int pressed_x;
                int pressed_y;
                int delta_x;
                int delta_y;
        };
        std::unordered_map<command::MouseButton, PressedMouseButton> m_mouse;
        int m_mouse_x = std::numeric_limits<int>::lowest();
        int m_mouse_y = std::numeric_limits<int>::lowest();

        const PressedMouseButton& pressed_mouse_button(command::MouseButton button) const
        {
                auto iter = m_mouse.find(button);
                if (iter != m_mouse.cend())
                {
                        return iter->second;
                }

                thread_local const PressedMouseButton m{};
                return m;
        }

        void clip_plane_show(double position)
        {
                m_clip_plane_view_matrix = m_camera.renderer_info().main_view_matrix;
                clip_plane_position(position);
        }

        void clip_plane_position(double position)
        {
                if (!m_clip_plane_view_matrix)
                {
                        error("Clip plane is not set");
                }

                ASSERT(position >= 0.0 && position <= 1.0);

                // Уравнение плоскости
                // -z = 0 или (0, 0, -1, 0).
                // Уравнение плоскости для исходных координат
                // (0, 0, -1, 0) * view matrix.
                vec4 plane = -m_clip_plane_view_matrix->row(2);

                vec3 n(plane[0], plane[1], plane[2]);
                double d = n.norm_1();

                // Уравнение плоскости со смещением
                // -z = d * (1 - 2 * position) или (0, 0, -1, d * (2 * position - 1)).
                plane[3] += d * (2 * position - 1);

                plane /= n.norm();

                m_renderer->set_clip_plane(plane);
        }

        void clip_plane_hide()
        {
                m_clip_plane_view_matrix.reset();
                m_renderer->set_clip_plane(std::nullopt);
        }

        void mouse_move(int x, int y)
        {
                for (auto& [button, m] : m_mouse)
                {
                        if (m.pressed)
                        {
                                m.delta_x = x - m_mouse_x;
                                m.delta_y = y - m_mouse_y;
                        }
                }
                m_mouse_x = x;
                m_mouse_y = y;

                //

                bool changed = false;

                const PressedMouseButton& right = pressed_mouse_button(command::MouseButton::Right);
                if (right.pressed && m_draw_rectangle.is_inside(right.pressed_x, right.pressed_y)
                    && (right.delta_x != 0 || right.delta_y != 0))
                {
                        m_camera.rotate(-right.delta_x, -right.delta_y);
                        changed = true;
                }

                const PressedMouseButton& left = pressed_mouse_button(command::MouseButton::Left);
                if (left.pressed && m_draw_rectangle.is_inside(left.pressed_x, left.pressed_y)
                    && (left.delta_x != 0 || left.delta_y != 0))
                {
                        m_camera.move(vec2(-left.delta_x, left.delta_y));
                        changed = true;
                }

                if (changed)
                {
                        m_renderer->set_camera(m_camera.renderer_info());
                }
        }

        void send(Command&& event) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // clang-format off
                const auto visitors =
                Visitors
                {
                [this](const command::UpdateMeshObject& d)
                {
                        if (auto ptr = d.object.lock(); ptr)
                        {
                                m_renderer->object_update(*ptr);
                        }

                },
                [this](const command::UpdateVolumeObject& d)
                {
                        if (auto ptr = d.object.lock(); ptr)
                        {
                                m_renderer->object_update(*ptr);
                        }
                },
                [this](const command::DeleteObject& d)
                {
                        m_renderer->object_delete(d.id);
                },
                [this](const command::ShowObject& d)
                {
                        m_renderer->object_show(d.id);
                },
                [this](const command::DeleteAllObjects&)
                {
                        m_renderer->object_delete_all();
                        reset_view_handler();
                },
                [this](const command::ResetView&)
                {
                        reset_view_handler();
                },
                [this](const command::SetAmbient& d)
                {
                        Color light = Color(d.value);
                        m_renderer->set_light_a(light);
                },
                [this](const command::SetDiffuse& d)
                {
                        Color light = Color(d.value);
                        m_renderer->set_light_d(light);
                },
                [this](const command::SetSpecular& d)
                {
                        Color light = Color(d.value);
                        m_renderer->set_light_s(light);
                },
                [this](const command::SetBackgroundColor& d)
                {
                        m_renderer->set_background_color(d.value);
                        bool background_is_dark = d.value.luminance() <= 0.5;
                        m_text->set_color(background_is_dark ? Color(1) : Color(0));
                },
                [this](const command::SetDefaultColor& d)
                {
                        m_renderer->set_default_color(d.value);
                },
                [this](const command::SetDefaultSpecularColor& d)
                {
                        m_renderer->set_default_specular_color(d.value);
                },
                [this](const command::SetWireframeColor& d)
                {
                        m_renderer->set_wireframe_color(d.value);
                },
                [this](const command::SetClipPlaneColor& d)
                {
                        m_renderer->set_clip_plane_color(d.value);
                },
                [this](const command::SetNormalLength& d)
                {
                        m_renderer->set_normal_length(d.value);
                },
                [this](const command::SetNormalColorPositive& d)
                {
                        m_renderer->set_normal_color_positive(d.value);
                },
                [this](const command::SetNormalColorNegative& d)
                {
                        m_renderer->set_normal_color_negative(d.value);
                },
                [this](const command::SetDefaultNs& d)
                {
                        m_renderer->set_default_ns(d.value);
                },
                [this](const command::ShowSmooth& d)
                {
                        m_renderer->set_show_smooth(d.show);
                },
                [this](const command::ShowWireframe& d)
                {
                        m_renderer->set_show_wireframe(d.show);
                },
                [this](const command::ShowShadow& d)
                {
                        m_renderer->set_show_shadow(d.show);
                },
                [this](const command::ShowFog& d)
                {
                        m_renderer->set_show_fog(d.show);
                },
                [this](const command::ShowMaterials& d)
                {
                        m_renderer->set_show_materials(d.show);
                },
                [this](const command::ShowFps& d)
                {
                        m_text_active = d.show;
                },
                [this](const command::ShowPencilSketch& d)
                {
                        m_pencil_sketch_active = d.show;
                },
                [this](const command::ShowDft& d)
                {
                        if (m_dft_active != d.show)
                        {
                                m_dft_active = d.show;
                                create_swapchain();
                        }
                },
                [this](const command::SetDftBrightness& d)
                {
                        m_dft->set_brightness(d.value);
                },
                [this](const command::SetDftBackgroundColor& d)
                {
                        m_dft->set_background_color(d.value);
                },
                [this](const command::SetDftColor& d)
                {
                        m_dft->set_color(d.value);
                },
                [this](const command::ShowConvexHull2D& d)
                {
                        m_convex_hull_active = d.show;
                        if (m_convex_hull_active)
                        {
                                m_convex_hull->reset_timer();
                        }
                },
                [this](const command::ShowOpticalFlow& d)
                {
                        m_optical_flow_active = d.show;
                },
                [this](const command::SetVerticalSync& d)
                {
                        set_vertical_sync_swapchain(d.enabled);
                },
                [this](const command::SetShadowZoom& d)
                {
                        m_renderer->set_shadow_zoom(d.value);
                },
                [this](const command::ClipPlaneShow& d)
                {
                        clip_plane_show(d.position);
                },
                [this](const command::ClipPlanePosition& d)
                {
                        clip_plane_position(d.position);
                },
                [this](const command::ClipPlaneHide&)
                {
                        clip_plane_hide();
                },
                [this](const command::ShowNormals& d)
                {
                        m_renderer->set_show_normals(d.show);
                },
                [this](const command::MousePress& d)
                {
                        m_mouse_x = d.x;
                        m_mouse_y = d.y;
                        PressedMouseButton& m = m_mouse[d.button];
                        m.pressed = true;
                        m.pressed_x = m_mouse_x;
                        m.pressed_y = m_mouse_y;
                        m.delta_x = 0;
                        m.delta_y = 0;
                },
                [this](const command::MouseRelease& d)
                {
                        m_mouse[d.button].pressed = false;
                        m_mouse_x = d.x;
                        m_mouse_y = d.y;
                },
                [this](const command::MouseMove& d)
                {
                        mouse_move(d.x, d.y);
                },
                [this](const command::MouseWheel& d)
                {
                        m_camera.scale(d.x - m_draw_rectangle.x0(), d.y - m_draw_rectangle.y0(), d.delta);
                        m_renderer->set_camera(m_camera.renderer_info());
                },
                [](const command::WindowResize&)
                {
                }
                };
                // clang-format on

                std::visit(visitors, event.data());
        }

        void receive(const std::vector<Info>& info) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                // clang-format off
                const auto visitors =
                Visitors
                {
                [this](info::Camera* d)
                {
                        *d = m_camera.view_info();
                }
                };
                // clang-format on

                for (const Info& v : info)
                {
                        std::visit(visitors, v.data());
                }
        }

        //

        void reset_view_handler()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_camera.reset(vec3(1, 0, 0), vec3(0, 1, 0), 1, vec2(0, 0));

                m_renderer->set_camera(m_camera.renderer_info());
        }

        void set_vertical_sync_swapchain(bool v)
        {
                if (v && m_present_mode != vulkan::PresentMode::PreferSync)
                {
                        m_present_mode = vulkan::PresentMode::PreferSync;
                        create_swapchain();
                        return;
                }
                if (!v && m_present_mode != vulkan::PresentMode::PreferFast)
                {
                        m_present_mode = vulkan::PresentMode::PreferFast;
                        create_swapchain();
                        return;
                }
        }

        void create_swapchain()
        {
                m_instance->device_wait_idle();

                m_text->delete_buffers();
                m_convex_hull->delete_buffers();
                m_pencil_sketch->delete_buffers();
                m_dft->delete_buffers();
                m_optical_flow->delete_buffers();
                m_renderer->delete_buffers();

                m_object_image.reset();
                m_resolve_command_buffers.reset();
                m_resolve_texture.reset();
                m_render_buffers.reset();
                m_swapchain.reset();

                const std::unordered_set<uint32_t> swapchain_family_indices = {
                        m_instance->graphics_compute_queues()[0].family_index(),
                        m_instance->presentation_queue().family_index()};

                m_swapchain = std::make_unique<vulkan::Swapchain>(
                        m_instance->surface(), m_instance->device(), swapchain_family_indices, VULKAN_SURFACE_FORMAT,
                        VULKAN_PREFERRED_IMAGE_COUNT, m_present_mode);

                constexpr RenderBufferCount buffer_count = RenderBufferCount::One;
                m_render_buffers = create_render_buffers(
                        buffer_count, *m_swapchain, m_instance->graphics_compute_command_pool(), m_instance->device(),
                        VULKAN_MINIMUM_SAMPLE_COUNT);

                //

                constexpr VkImageLayout RESOLVE_TEXTURE_IMAGE_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                m_resolve_texture = std::make_unique<vulkan::ImageWithMemory>(
                        m_instance->device(), m_instance->graphics_compute_command_pool(),
                        m_instance->graphics_compute_queues()[0],
                        std::unordered_set({m_instance->graphics_compute_command_pool().family_index()}),
                        std::vector<VkFormat>({m_swapchain->format()}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(m_swapchain->width(), m_swapchain->height()), RESOLVE_TEXTURE_IMAGE_LAYOUT,
                        false /*storage*/);

                ASSERT(m_resolve_texture->usage() & VK_IMAGE_USAGE_TRANSFER_DST_BIT);
                ASSERT(m_resolve_texture->usage() & VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT(!(m_resolve_texture->usage() & VK_IMAGE_USAGE_STORAGE_BIT));

                //

                m_object_image = std::make_unique<vulkan::ImageWithMemory>(
                        m_instance->device(), m_instance->graphics_compute_command_pool(),
                        m_instance->graphics_compute_queues()[0],
                        std::unordered_set({m_instance->graphics_compute_queues()[0].family_index()}),
                        std::vector<VkFormat>({VULKAN_OBJECT_IMAGE_FORMAT}), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(m_swapchain->width(), m_swapchain->height()), VK_IMAGE_LAYOUT_GENERAL,
                        true /*storage*/);

                ASSERT(m_object_image->usage() & VK_IMAGE_USAGE_STORAGE_BIT);

                //

                m_text->create_buffers(
                        &m_render_buffers->buffers_2d(),
                        Region<2, int>(0, 0, m_swapchain->width(), m_swapchain->height()));

                //

                Region<2, int> w_2;
                const bool two_windows = window_position_and_size(
                        m_dft_active, m_swapchain->width(), m_swapchain->height(), m_frame_size_in_pixels,
                        &m_draw_rectangle, &w_2);

                ASSERT(m_draw_rectangle.x0() >= 0 && m_draw_rectangle.y0() >= 0);
                ASSERT(m_draw_rectangle.width() > 0 && m_draw_rectangle.height() > 0);
                ASSERT(m_draw_rectangle.x1() <= static_cast<int>(m_swapchain->width()));
                ASSERT(m_draw_rectangle.y1() <= static_cast<int>(m_swapchain->height()));

                //

                m_resolve_command_buffers = std::make_unique<vulkan::CommandBuffers>(create_command_buffers_resolve(
                        m_instance->device(), m_instance->graphics_compute_command_pool(), m_render_buffers->images(),
                        m_render_buffers->image_layout(), std::vector<VkImage>({m_resolve_texture->image()}),
                        RESOLVE_TEXTURE_IMAGE_LAYOUT, m_draw_rectangle));

                //

                m_renderer->create_buffers(
                        m_swapchain.get(), &m_render_buffers->buffers_3d(), m_object_image.get(), m_draw_rectangle);

                //

                m_convex_hull->create_buffers(&m_render_buffers->buffers_2d(), *m_object_image, m_draw_rectangle);

                m_pencil_sketch->create_buffers(
                        &m_render_buffers->buffers_2d(), *m_resolve_texture, *m_object_image, m_draw_rectangle);

                m_optical_flow->create_buffers(
                        &m_render_buffers->buffers_2d(), *m_resolve_texture, m_window_ppi, m_draw_rectangle);

                if (two_windows)
                {
                        ASSERT(w_2.x0() >= 0 && w_2.y0() >= 0);
                        ASSERT(w_2.width() > 0 && w_2.height() > 0);
                        ASSERT(w_2.x1() <= static_cast<int>(m_swapchain->width()));
                        ASSERT(w_2.y1() <= static_cast<int>(m_swapchain->height()));

                        m_dft->create_buffers(
                                &m_render_buffers->buffers_2d(), *m_resolve_texture, m_draw_rectangle, w_2);
                }

                //

                m_camera.resize(m_draw_rectangle.width(), m_draw_rectangle.height());
                m_renderer->set_camera(m_camera.renderer_info());
        }

        VkSemaphore resolve_to_texture(
                const vulkan::Queue& graphics_queue,
                VkSemaphore wait_semaphore,
                unsigned image_index) const
        {
                ASSERT(m_resolve_command_buffers->count() == 1 || image_index < m_resolve_command_buffers->count());

                const unsigned index = m_resolve_command_buffers->count() == 1 ? 0 : image_index;

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, (*m_resolve_command_buffers)[index],
                        *m_resolve_semaphore, graphics_queue);

                return *m_resolve_semaphore;
        }

        bool render() const
        {
                static_assert(!std::remove_reference_t<decltype(m_instance->graphics_compute_queues())>().empty());

                uint32_t image_index;
                if (!vulkan::acquire_next_image(
                            m_instance->device(), m_swapchain->swapchain(), *m_image_semaphore, &image_index))
                {
                        return false;
                }

                VkSemaphore wait_semaphore;

                const vulkan::Queue& graphics_queue = m_instance->graphics_compute_queues()[0];
                const vulkan::Queue& compute_queue = m_instance->compute_queue();

                wait_semaphore = m_renderer->draw(graphics_queue, image_index);

                if (m_pencil_sketch_active)
                {
                        wait_semaphore = resolve_to_texture(graphics_queue, wait_semaphore, image_index);
                        wait_semaphore = m_pencil_sketch->draw(graphics_queue, wait_semaphore, image_index);
                }

                if (m_dft_active || m_optical_flow_active)
                {
                        wait_semaphore = resolve_to_texture(graphics_queue, wait_semaphore, image_index);
                }

                if (m_dft_active)
                {
                        wait_semaphore = m_dft->draw(graphics_queue, wait_semaphore, image_index);
                }

                if (m_optical_flow_active)
                {
                        wait_semaphore =
                                m_optical_flow->draw(graphics_queue, compute_queue, wait_semaphore, image_index);
                }

                if (m_convex_hull_active)
                {
                        wait_semaphore = m_convex_hull->draw(graphics_queue, wait_semaphore, image_index);
                }

                if (m_text_active)
                {
                        wait_semaphore =
                                m_text->draw(graphics_queue, wait_semaphore, image_index, m_frame_rate.text_data());
                }

                wait_semaphore = m_render_buffers->resolve_to_swapchain(
                        graphics_queue, *m_image_semaphore, wait_semaphore, image_index);

                if (!vulkan::queue_present(
                            wait_semaphore, m_swapchain->swapchain(), image_index, m_instance->presentation_queue()))
                {
                        return false;
                }

                vulkan::queue_wait_idle(graphics_queue);

                return true;
        }

public:
        Impl(const WindowID& window, double window_ppi) : m_window_ppi(window_ppi)
        {
                if (window_ppi <= 0)
                {
                        error("Window PPI is not positive");
                }

                // Этот цвет меняется в set_background_color
                constexpr Srgb8 text_color(255, 255, 255);

                m_present_mode = VULKAN_DEFAULT_PRESENT_MODE;

                {
                        const std::vector<std::string> instance_extensions =
                                merge<std::string>(vulkan_create_surface_extensions());

                        const std::vector<std::string> device_extensions = {};

                        const std::vector<vulkan::PhysicalDeviceFeatures> required_features =
                                merge<vulkan::PhysicalDeviceFeatures>(
                                        gpu::convex_hull::View::required_device_features(),
                                        gpu::dft::View::required_device_features(),
                                        gpu::optical_flow::View::required_device_features(),
                                        gpu::pencil_sketch::View::required_device_features(),
                                        gpu::renderer::Renderer::required_device_features(),
                                        gpu::text_writer::View::required_device_features(),
                                        device_features_sample_shading(
                                                VULKAN_MINIMUM_SAMPLE_COUNT, VULKAN_SAMPLE_SHADING),
                                        device_features_sampler_anisotropy(VULKAN_SAMPLER_ANISOTROPY));

                        const std::vector<vulkan::PhysicalDeviceFeatures> optional_features = {};

                        const std::function<VkSurfaceKHR(VkInstance)> surface_function = [&](VkInstance instance) {
                                return vulkan_create_surface(window, instance);
                        };

                        m_instance = std::make_unique<vulkan::VulkanInstance>(
                                instance_extensions, device_extensions, required_features, optional_features,
                                surface_function);
                }

                ASSERT(m_instance->graphics_compute_command_pool().family_index()
                       == m_instance->graphics_compute_queues()[0].family_index());
                ASSERT(m_instance->compute_command_pool().family_index() == m_instance->compute_queue().family_index());
                ASSERT(m_instance->transfer_command_pool().family_index()
                       == m_instance->transfer_queue().family_index());

                m_image_semaphore = std::make_unique<vulkan::Semaphore>(m_instance->device());
                m_resolve_semaphore = std::make_unique<vulkan::Semaphore>(m_instance->device());

                const vulkan::Queue& graphics_compute_queue = m_instance->graphics_compute_queues()[0];
                const vulkan::CommandPool& graphics_compute_command_pool = m_instance->graphics_compute_command_pool();
                const vulkan::Queue& compute_queue = m_instance->compute_queue();
                const vulkan::CommandPool& compute_command_pool = m_instance->compute_command_pool();
                const vulkan::Queue& transfer_queue = m_instance->transfer_queue();
                const vulkan::CommandPool& transfer_command_pool = m_instance->transfer_command_pool();

                m_renderer = gpu::renderer::create_renderer(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, VULKAN_SAMPLE_SHADING, VULKAN_SAMPLER_ANISOTROPY);

                m_text = gpu::text_writer::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, VULKAN_SAMPLE_SHADING, m_frame_rate.text_size(), text_color);

                m_convex_hull = gpu::convex_hull::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, VULKAN_SAMPLE_SHADING);

                m_pencil_sketch = gpu::pencil_sketch::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, VULKAN_SAMPLE_SHADING);

                m_dft = gpu::dft::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, transfer_command_pool,
                        transfer_queue, VULKAN_SAMPLE_SHADING);

                m_optical_flow = gpu::optical_flow::create_view(
                        *m_instance, graphics_compute_command_pool, graphics_compute_queue, compute_command_pool,
                        compute_queue, transfer_command_pool, transfer_queue, VULKAN_SAMPLE_SHADING);

                //

                create_swapchain();

                //

                reset_view_handler();
                clip_plane_hide();
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void loop(const std::function<void()>& dispatch_events, std::atomic_bool* stop)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                double last_frame_time = time_in_seconds();
                while (!(*stop))
                {
                        dispatch_events();

                        if (m_text_active)
                        {
                                m_frame_rate.calculate();
                        }

                        if (!render())
                        {
                                create_swapchain();
                                continue;
                        }

                        if (m_renderer->empty())
                        {
                                sleep_this_thread_until(last_frame_time + IDLE_MODE_FRAME_DURATION_IN_SECONDS);
                                last_frame_time = time_in_seconds();
                        }
                }
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

std::unique_ptr<View> create_view_impl(
        WindowID parent_window,
        double parent_window_ppi,
        std::vector<Command>&& initial_commands)
{
        return std::make_unique<ViewThread<Impl>>(parent_window, parent_window_ppi, std::move(initial_commands));
}
}
