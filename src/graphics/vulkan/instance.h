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

#include "buffers.h"
#include "descriptor.h"
#include "objects.h"
#include "physical_device.h"
#include "shader.h"

#include "com/error.h"
#include "com/span.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vulkan
{
class SwapChain
{
        VkDevice m_device;
        VkCommandPool m_command_pool;
        VkDescriptorSet m_descriptor_set;
        VkExtent2D m_extent;

        SwapChainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;

        std::vector<ImageView> m_image_views;

        RenderPass m_render_pass;
        PipelineLayout m_pipeline_layout;
        Pipeline m_pipeline;

        std::vector<Framebuffer> m_framebuffers;

        CommandBuffers m_command_buffers;

public:
        SwapChain(VkSurfaceKHR surface, VkPhysicalDevice physical_device, const std::vector<uint32_t> family_indices,
                  VkDevice device, VkCommandPool command_pool, const std::vector<const vulkan::Shader*>& shaders,
                  const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                  const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                  VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet descriptor_set);

        SwapChain(const SwapChain&) = delete;
        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        void create_command_buffers(VkBuffer vertex_buffer, VkBuffer vertex_index_buffer, VkIndexType vertex_index_type,
                                    uint32_t vertex_count);
        bool command_buffers_created() const;

        VkSwapchainKHR swap_chain() const noexcept;
        const VkCommandBuffer& command_buffer(uint32_t index) const noexcept;
};

class VulkanInstance
{
        static constexpr unsigned MAX_FRAMES_IN_FLIGHT = 2;

        unsigned m_current_frame = 0;

        Instance m_instance;
        std::optional<DebugReportCallback> m_callback;

        SurfaceKHR m_surface;

        PhysicalDevice m_physical_device;

        Device m_device;

        std::vector<Semaphore> m_image_available_semaphores;
        std::vector<Semaphore> m_render_finished_semaphores;
        std::vector<Fence> m_in_flight_fences;

        CommandPool m_graphics_command_pool;
        VkQueue m_graphics_queue = VK_NULL_HANDLE;

        CommandPool m_transfer_command_pool;
        VkQueue m_transfer_queue = VK_NULL_HANDLE;

        VkQueue m_compute_queue = VK_NULL_HANDLE;
        VkQueue m_presentation_queue = VK_NULL_HANDLE;

        std::vector<uint32_t> m_vertex_buffer_family_indices;
        std::vector<uint32_t> m_image_family_indices;

        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;
        std::vector<VkVertexInputBindingDescription> m_vertex_binding_descriptions;
        std::vector<VkVertexInputAttributeDescription> m_vertex_attribute_descriptions;
        //
        uint32_t m_vertex_count;
        VertexBufferWithDeviceLocalMemory m_vertex_buffer;
        IndexBufferWithDeviceLocalMemory m_vertex_index_buffer;
        VkIndexType m_vertex_index_type;

        DescriptorWithBuffers m_descriptor_with_buffers;

        std::optional<SwapChain> m_swapchain;

        void create_swap_chain();
        void recreate_swap_chain();

public:
        VulkanInstance(int api_version_major, int api_version_minor, const std::vector<std::string>& required_instance_extensions,
                       const std::vector<std::string>& required_device_extensions,
                       const std::vector<std::string>& required_validation_layers,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface,
                       const Span<const uint32_t>& vertex_shader_code, const Span<const uint32_t>& fragment_shader_code,
                       const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                       const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions, uint32_t vertex_count,
                       size_t vertex_data_size, const void* vertex_data, size_t vertex_index_data_size,
                       const void* vertex_index_data,
                       const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                       const std::vector<VkDeviceSize>& descriptor_set_layout_bindings_sizes);

        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance(VulkanInstance&&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;
        VulkanInstance& operator=(VulkanInstance&&) = delete;

        VkInstance instance() const noexcept;

        void draw_frame();

        void device_wait_idle() const;

        void copy_to_buffer(uint32_t index, const Span<const void>& data) const;
};
}

#endif
