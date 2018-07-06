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

#pragma once

#if defined(VULKAN_FOUND)

#include "com/span.h"
#include "graphics/vulkan/objects.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vulkan
{
struct PhysicalDevice
{
        const VkPhysicalDevice physical_device;
        const unsigned graphics_family_index;
        const unsigned compute_family_index;
        const unsigned presentation_family_index;
        const VkSurfaceCapabilitiesKHR surface_capabilities;
        const std::vector<VkSurfaceFormatKHR> surface_formats;
        const std::vector<VkPresentModeKHR> present_modes;
};

class SwapChain
{
        SwapChainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;

        std::vector<ImageView> m_image_views;

        RenderPass m_render_pass;
        PipelineLayout m_pipeline_layout;
        Pipeline m_pipeline;

        std::vector<Framebuffer> m_framebuffers;

        CommandPool m_command_pool;

        std::vector<VkCommandBuffer> m_command_buffers;

public:
        SwapChain(VkDevice device, VkSurfaceKHR surface, const std::vector<VkSurfaceFormatKHR>& surface_formats,
                  const std::vector<VkPresentModeKHR>& present_modes, const VkSurfaceCapabilitiesKHR& surface_capabilities,
                  unsigned graphics_family_index, unsigned presentation_family_index,
                  const std::vector<const vulkan::Shader*>& shaders);

        SwapChain(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        VkSwapchainKHR swap_chain() const noexcept;
        const std::vector<VkCommandBuffer>& command_buffers() const noexcept;
};

class VulkanInstance
{
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        Instance m_instance;
        std::optional<DebugReportCallback> m_callback;

        SurfaceKHR m_surface;

        PhysicalDevice m_physical_device;

        Device m_device;

        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

        SwapChain m_swapchain;

        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_compute_queue = VK_NULL_HANDLE;
        VkQueue m_presentation_queue = VK_NULL_HANDLE;

        std::vector<Semaphore> m_image_available_semaphores;
        std::vector<Semaphore> m_render_finished_semaphores;
        std::vector<Fence> m_in_flight_fences;

        unsigned m_current_frame = 0;

public:
        VulkanInstance(int api_version_major, int api_version_minor, const std::vector<std::string>& required_instance_extensions,
                       const std::vector<std::string>& required_device_extensions,
                       const std::vector<std::string>& required_validation_layers,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface,
                       const Span<const uint32_t>& vertex_shader_code, const Span<const uint32_t>& fragment_shader_code);

        ~VulkanInstance();

        VkInstance instance() const;

        void draw_frame();

        void device_wait_idle() const;
};
}

#endif
