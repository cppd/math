/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "canvas.h"

#include "com/log.h"
#include "gpgpu/convex_hull/show/vulkan/ch_show.h"
#include "graphics/vulkan/objects.h"
#include "text/vulkan/text.h"

#include <thread>

constexpr Srgb8 TEXT_COLOR(255, 255, 255);

namespace
{
class Canvas final : public VulkanCanvas
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;

        bool m_text_active = true;
        bool m_convex_hull_active = true;

        std::unique_ptr<VulkanText> m_text;
        std::unique_ptr<gpgpu_vulkan::ConvexHullShow> m_convex_hull;

        vulkan::Semaphore m_text_semaphore;
        vulkan::Semaphore m_convex_hull_semaphore;

        void set_text_color(const Color& c) override
        {
                m_text->set_color(c);
        }

        void set_text_active(bool v) override
        {
                m_text_active = v;
        }

        void set_pencil_sketch_active(bool /*v*/) override
        {
        }

        bool pencil_sketch_active() override
        {
                return false;
        }

        void set_dft_active(bool /*v*/) override
        {
        }

        bool dft_active() override
        {
                return false;
        }

        void set_dft_brightness(double /*v*/) override
        {
        }

        void set_dft_background_color(const Color& /*c*/) override
        {
        }

        void set_dft_color(const Color& /*c*/) override
        {
        }

        void set_convex_hull_active(bool v) override
        {
                m_convex_hull_active = v;
                if (m_convex_hull_active)
                {
                        m_convex_hull->reset_timer();
                }
        }

        void set_optical_flow_active(bool /*v*/) override
        {
        }

        void create_buffers(const vulkan::Swapchain* swapchain, vulkan::RenderBuffers2D* render_buffers, const mat4& matrix,
                            const vulkan::StorageImage& objects) override;
        void delete_buffers() override;

        VkSemaphore draw(VkQueue graphics_queue, VkSemaphore wait_semaphore, unsigned image_index,
                         const TextData& text_data) override
        {
                VkSemaphore signal_semaphore;

                if (m_convex_hull_active)
                {
                        m_convex_hull->draw(graphics_queue, wait_semaphore, m_convex_hull_semaphore, image_index);
                        wait_semaphore = m_convex_hull_semaphore;
                        signal_semaphore = m_convex_hull_semaphore;
                }
                else
                {
                        signal_semaphore = wait_semaphore;
                }

                if (m_text_active)
                {
                        m_text->draw(graphics_queue, wait_semaphore, m_text_semaphore, image_index, text_data);
                        wait_semaphore = m_text_semaphore;
                        signal_semaphore = m_text_semaphore;
                }
                else
                {
                        signal_semaphore = wait_semaphore;
                }

                return signal_semaphore;
        }

public:
        Canvas(const vulkan::VulkanInstance& instance, bool sample_shading, int text_size)
                : m_instance(instance),
                  m_text(create_vulkan_text(instance, sample_shading, text_size, TEXT_COLOR)),
                  m_convex_hull(gpgpu_vulkan::create_convex_hull_show(instance, sample_shading)),
                  m_text_semaphore(instance.device()),
                  m_convex_hull_semaphore(instance.device())
        {
        }

        ~Canvas() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                try
                {
                        m_instance.device_wait_idle();
                }
                catch (std::exception& e)
                {
                        LOG(std::string("Device wait idle exception in the Vulkan canvas destructor: ") + e.what());
                }
                catch (...)
                {
                        LOG("Device wait idle unknown exception in the Vulkan canvas destructor");
                }
        }
};

void Canvas::create_buffers(const vulkan::Swapchain* /*swapchain*/, vulkan::RenderBuffers2D* render_buffers, const mat4& matrix,
                            const vulkan::StorageImage& objects)
{
        m_text->create_buffers(render_buffers, matrix);
        m_convex_hull->create_buffers(render_buffers, matrix, objects);
}

void Canvas::delete_buffers()
{
        m_text->delete_buffers();
        m_convex_hull->delete_buffers();
}
}

std::vector<vulkan::PhysicalDeviceFeatures> VulkanCanvas::required_device_features()
{
        return gpgpu_vulkan::ConvexHullShow::required_device_features();
}

std::unique_ptr<VulkanCanvas> create_vulkan_canvas(const vulkan::VulkanInstance& instance, bool sample_shading, int text_size)
{
        return std::make_unique<Canvas>(instance, sample_shading, text_size);
}
