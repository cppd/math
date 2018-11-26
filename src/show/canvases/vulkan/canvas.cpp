/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "text/vulkan/text.h"

constexpr Srgb8 TEXT_COLOR(255, 255, 255);

namespace
{
class Canvas final : public VulkanCanvas
{
        bool m_text_active = true;

        std::unique_ptr<VulkanText> m_text;

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

        void set_convex_hull_active(bool /*v*/) override
        {
        }

        void set_optical_flow_active(bool /*v*/) override
        {
        }

        void create_buffers(const vulkan::Swapchain* swapchain, const mat4& matrix) override;
        void delete_buffers() override;

        bool text_active() const noexcept override
        {
                return m_text_active;
        }
        void draw_text(VkFence queue_fence, VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore finished_semaphore,
                       unsigned image_index, int step_y, int x, int y, const std::vector<std::string>& text) override
        {
                m_text->draw(queue_fence, graphics_queue, wait_semaphore, finished_semaphore, image_index, step_y, x, y, text);
        }

public:
        Canvas(const vulkan::VulkanInstance& instance, int text_size)
                : m_text(create_vulkan_text(instance, text_size, TEXT_COLOR))
        {
        }
};

void Canvas::create_buffers(const vulkan::Swapchain* swapchain, const mat4& matrix)
{
        m_text->create_buffers(swapchain, matrix);
}

void Canvas::delete_buffers()
{
        m_text->delete_buffers();
}
}

std::unique_ptr<VulkanCanvas> create_vulkan_canvas(const vulkan::VulkanInstance& instance, int text_size)
{
        return std::make_unique<Canvas>(instance, text_size);
}
