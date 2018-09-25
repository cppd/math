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

#include "buffers.h"
#include "objects.h"
#include "physical_device.h"
#include "shader.h"

#include "com/color/color.h"
#include "com/error.h"
#include "com/type_detect.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vulkan
{
class SwapChain
{
        VkDevice m_device;
        VkCommandPool m_graphics_command_pool;
        VkExtent2D m_extent;

        VkSampleCountFlagBits m_sample_count_bit;

        SwapChainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;
        std::vector<ImageView> m_swap_chain_image_views;

        std::unique_ptr<DepthAttachment> m_depth_attachment;

        std::unique_ptr<ColorAttachment> m_multisampling_color_attachment;
        std::unique_ptr<DepthAttachment> m_multisampling_depth_attachment;

        RenderPass m_render_pass;
        std::vector<Framebuffer> m_framebuffers;

        PipelineLayout m_pipeline_layout;
        Pipeline m_pipeline;

        CommandBuffers m_command_buffers;

public:
        SwapChain(VkSurfaceKHR surface, VkPhysicalDevice physical_device,
                  const std::vector<uint32_t>& swap_chain_image_family_indices,
                  const std::vector<uint32_t>& depth_image_family_indices, const Device& device,
                  VkCommandPool graphics_command_pool, VkQueue graphics_queue, int preferred_image_count,
                  int required_minimum_sample_count, const std::vector<const vulkan::Shader*>& shaders,
                  const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                  const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                  const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        SwapChain(SwapChain&&) = default;
        ~SwapChain() = default;

        //

        void create_command_buffers(const Color& clear_color,
                                    const std::function<void(VkPipelineLayout pipeline_layout, VkPipeline pipeline,
                                                             VkCommandBuffer command_buffer)>& commands_for_triangle_topology);
        void delete_command_buffers();
        bool command_buffers_created() const;

        VkSwapchainKHR swap_chain() const noexcept;
        const VkCommandBuffer& command_buffer(uint32_t index) const noexcept;
};

class VulkanInstance
{
        unsigned m_current_frame = 0;

        Instance m_instance;
        std::optional<DebugReportCallback> m_callback;

        SurfaceKHR m_surface;

        PhysicalDevice m_physical_device;

        Device m_device;

        unsigned m_max_frames_in_flight;
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
        std::vector<uint32_t> m_swap_chain_image_family_indices;
        std::vector<uint32_t> m_texture_image_family_indices;
        std::vector<uint32_t> m_depth_image_family_indices;

public:
        VulkanInstance(int api_version_major, int api_version_minor, const std::vector<std::string>& required_instance_extensions,
                       const std::vector<std::string>& required_device_extensions,
                       const std::vector<std::string>& required_validation_layers,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface, unsigned max_frames_in_flight);

        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance(VulkanInstance&&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;
        VulkanInstance& operator=(VulkanInstance&&) = delete;

        //

        VkInstance instance() const noexcept;
        const Device& device() const noexcept;

        SwapChain create_swap_chain(int preferred_image_count, int required_minimum_sample_count,
                                    const std::vector<const vulkan::Shader*>& shaders,
                                    const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
                                    const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions,
                                    const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

        template <typename T>
        VertexBufferWithDeviceLocalMemory create_vertex_buffer(const T& data) const
        {
                static_assert(is_vector<T> || is_array<T>);
                return VertexBufferWithDeviceLocalMemory(m_device, m_transfer_command_pool, m_transfer_queue,
                                                         m_vertex_buffer_family_indices,
                                                         data.size() * sizeof(typename T::value_type), data.data());
        }

        template <typename T>
        IndexBufferWithDeviceLocalMemory create_index_buffer(const T& data) const
        {
                static_assert(is_vector<T> || is_array<T>);
                return IndexBufferWithDeviceLocalMemory(m_device, m_transfer_command_pool, m_transfer_queue,
                                                        m_vertex_buffer_family_indices,
                                                        data.size() * sizeof(typename T::value_type), data.data());
        }

        template <typename T>
        Texture create_texture(uint32_t width, uint32_t height, const std::vector<T>& rgba_pixels) const
        {
                return Texture(m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool, m_transfer_queue,
                               m_texture_image_family_indices, width, height, rgba_pixels);
        }

        [[nodiscard]] bool draw_frame(SwapChain& swap_chain);

        void device_wait_idle() const;
};
}
