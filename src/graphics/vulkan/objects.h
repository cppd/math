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

#pragma once

#include "com/span.h"

#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
class Instance final
{
        VkInstance m_instance = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        Instance();
        Instance(const VkInstanceCreateInfo& create_info);
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&&) noexcept;
        Instance& operator=(Instance&&) noexcept;

        operator VkInstance() const noexcept;
};

class DebugReportCallback final
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_callback = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DebugReportCallback* from) noexcept;

public:
        DebugReportCallback();
        DebugReportCallback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT& create_info);
        ~DebugReportCallback();

        DebugReportCallback(const DebugReportCallback&) = delete;
        DebugReportCallback& operator=(const DebugReportCallback&) = delete;

        DebugReportCallback(DebugReportCallback&&) noexcept;
        DebugReportCallback& operator=(DebugReportCallback&&) noexcept;

        operator VkDebugReportCallbackEXT() const noexcept;
};

class Device final
{
        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDeviceFeatures m_features = {};

        void destroy() noexcept;
        void move(Device* from) noexcept;

public:
        Device();
        Device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        Device(Device&&) noexcept;
        Device& operator=(Device&&) noexcept;

        operator VkDevice() const noexcept;

        VkPhysicalDevice physical_device() const noexcept;

        const VkPhysicalDeviceFeatures& features() const noexcept;

        uint32_t physical_device_memory_type_index(uint32_t memory_type_bits, VkMemoryPropertyFlags memory_property_flags) const;
};

class SurfaceKHR final
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SurfaceKHR* from) noexcept;

public:
        SurfaceKHR();
        SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
        ~SurfaceKHR();

        SurfaceKHR(const SurfaceKHR&) = delete;
        SurfaceKHR& operator=(const SurfaceKHR&) = delete;

        SurfaceKHR(SurfaceKHR&&) noexcept;
        SurfaceKHR& operator=(SurfaceKHR&&) noexcept;

        operator VkSurfaceKHR() const noexcept;
};

class SwapchainKHR final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SwapchainKHR* from) noexcept;

public:
        SwapchainKHR();
        SwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info);
        ~SwapchainKHR();

        SwapchainKHR(const SwapchainKHR&) = delete;
        SwapchainKHR& operator=(const SwapchainKHR&) = delete;

        SwapchainKHR(SwapchainKHR&&) noexcept;
        SwapchainKHR& operator=(SwapchainKHR&&) noexcept;

        operator VkSwapchainKHR() const noexcept;
};

class ImageView final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkImageView m_image_view = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ImageView* from) noexcept;

public:
        ImageView();
        ImageView(VkDevice device, const VkImageViewCreateInfo& create_info);
        ~ImageView();

        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;

        ImageView(ImageView&&) noexcept;
        ImageView& operator=(ImageView&&) noexcept;

        operator VkImageView() const noexcept;
};

class ShaderModule final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkShaderModule m_shader_module = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        ShaderModule();
        ShaderModule(VkDevice device, const Span<const uint32_t>& code);
        ~ShaderModule();

        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;

        ShaderModule(ShaderModule&&) noexcept;
        ShaderModule& operator=(ShaderModule&&) noexcept;

        operator VkShaderModule() const noexcept;
};

class RenderPass final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkRenderPass m_render_pass = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(RenderPass* from) noexcept;

public:
        RenderPass();
        RenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info);
        ~RenderPass();

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        RenderPass(RenderPass&&) noexcept;
        RenderPass& operator=(RenderPass&&) noexcept;

        operator VkRenderPass() const noexcept;
};

class PipelineLayout final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(PipelineLayout* from) noexcept;

public:
        PipelineLayout();
        PipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info);
        ~PipelineLayout();

        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout& operator=(const PipelineLayout&) = delete;

        PipelineLayout(PipelineLayout&&) noexcept;
        PipelineLayout& operator=(PipelineLayout&&) noexcept;

        operator VkPipelineLayout() const noexcept;
};

class Pipeline final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Pipeline* from) noexcept;

public:
        Pipeline();
        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info);
        Pipeline(VkDevice device, const VkComputePipelineCreateInfo& create_info);
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(Pipeline&&) noexcept;
        Pipeline& operator=(Pipeline&&) noexcept;

        operator VkPipeline() const noexcept;
};

class Framebuffer final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Framebuffer* from) noexcept;

public:
        Framebuffer();
        Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info);
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(Framebuffer&&) noexcept;

        operator VkFramebuffer() const noexcept;
};

class CommandPool final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkCommandPool m_command_pool = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(CommandPool* from) noexcept;

public:
        CommandPool();
        CommandPool(VkDevice device, const VkCommandPoolCreateInfo& create_info);
        ~CommandPool();

        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        CommandPool(CommandPool&&) noexcept;
        CommandPool& operator=(CommandPool&&) noexcept;

        operator VkCommandPool() const noexcept;
};

class Semaphore final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkSemaphore m_semaphore = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Semaphore* from) noexcept;

public:
        Semaphore();
        Semaphore(VkDevice device);
        ~Semaphore();

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&&) noexcept;
        Semaphore& operator=(Semaphore&&) noexcept;

        operator VkSemaphore() const noexcept;
};

class Fence final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkFence m_fence = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Fence* from) noexcept;

public:
        Fence();
        Fence(VkDevice device, bool signaled);
        ~Fence();

        Fence(const Fence&) = delete;
        Fence& operator=(const Fence&) = delete;

        Fence(Fence&&) noexcept;
        Fence& operator=(Fence&&) noexcept;

        operator VkFence() const noexcept;
};

class BufferHandle final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkBuffer m_buffer = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(BufferHandle* from) noexcept;

public:
        BufferHandle();
        BufferHandle(VkDevice device, const VkBufferCreateInfo& create_info);
        ~BufferHandle();

        BufferHandle(const BufferHandle&) = delete;
        BufferHandle& operator=(const BufferHandle&) = delete;

        BufferHandle(BufferHandle&&) noexcept;
        BufferHandle& operator=(BufferHandle&&) noexcept;

        operator VkBuffer() const noexcept
        {
                return m_buffer;
        }
        VkDevice device() const noexcept
        {
                return m_device;
        }
};

class Buffer final
{
        BufferHandle m_buffer;
        VkDeviceSize m_size;
        VkBufferUsageFlags m_usage;

public:
        Buffer() = default;

        Buffer(VkDevice device, const VkBufferCreateInfo& create_info)
                : m_buffer(device, create_info), m_size(create_info.size), m_usage(create_info.usage)
        {
        }

        operator VkBuffer() const noexcept
        {
                return m_buffer;
        }
        VkDevice device() const noexcept
        {
                return m_buffer.device();
        }
        VkDeviceSize size() const noexcept
        {
                return m_size;
        }
        bool usage(VkBufferUsageFlagBits flag) const noexcept
        {
                return (m_usage & flag) == flag;
        }
};

class DeviceMemory final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkDeviceMemory m_device_memory = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DeviceMemory* from) noexcept;

public:
        DeviceMemory();
        DeviceMemory(VkDevice device, const VkMemoryAllocateInfo& allocate_info);
        ~DeviceMemory();

        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory& operator=(const DeviceMemory&) = delete;

        DeviceMemory(DeviceMemory&&) noexcept;
        DeviceMemory& operator=(DeviceMemory&&) noexcept;

        operator VkDeviceMemory() const noexcept
        {
                return m_device_memory;
        }
        VkDevice device() const noexcept
        {
                return m_device;
        }
};

class CommandBuffer final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkCommandPool m_command_pool = VK_NULL_HANDLE;
        VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(CommandBuffer* from) noexcept;

public:
        CommandBuffer();
        CommandBuffer(VkDevice device, VkCommandPool command_pool);
        ~CommandBuffer();

        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;

        CommandBuffer(CommandBuffer&&) noexcept;
        CommandBuffer& operator=(CommandBuffer&&) noexcept;

        operator VkCommandBuffer() const noexcept;
};

class CommandBuffers final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkCommandPool m_command_pool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_command_buffers;

        void destroy() noexcept;
        void move(CommandBuffers* from) noexcept;

public:
        CommandBuffers();
        CommandBuffers(VkDevice device, VkCommandPool command_pool, uint32_t count);
        ~CommandBuffers();

        CommandBuffers(const CommandBuffers&) = delete;
        CommandBuffers& operator=(const CommandBuffers&) = delete;

        CommandBuffers(CommandBuffers&&) noexcept;
        CommandBuffers& operator=(CommandBuffers&&) noexcept;

        const VkCommandBuffer& operator[](uint32_t index) const noexcept;
        uint32_t count() const noexcept;
        const std::vector<VkCommandBuffer>& buffers() const noexcept;
};

class DescriptorSetLayout final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorSetLayout* from) noexcept;

public:
        DescriptorSetLayout();
        DescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& create_info);
        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        DescriptorSetLayout(DescriptorSetLayout&&) noexcept;
        DescriptorSetLayout& operator=(DescriptorSetLayout&&) noexcept;

        operator VkDescriptorSetLayout() const noexcept;
};

class DescriptorPool final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorPool* from) noexcept;

public:
        DescriptorPool();
        DescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& create_info);
        ~DescriptorPool();

        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        DescriptorPool(DescriptorPool&&) noexcept;
        DescriptorPool& operator=(DescriptorPool&&) noexcept;

        operator VkDescriptorPool() const noexcept;
};

class DescriptorSet final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptor_set = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorSet* from) noexcept;

public:
        DescriptorSet();
        DescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);
        ~DescriptorSet();

        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;

        DescriptorSet(DescriptorSet&&) noexcept;
        DescriptorSet& operator=(DescriptorSet&&) noexcept;

        operator VkDescriptorSet() const noexcept;
};

class DescriptorSets final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptor_sets;

        void destroy() noexcept;
        void move(DescriptorSets* from) noexcept;

public:
        DescriptorSets();
        DescriptorSets(VkDevice device, VkDescriptorPool descriptor_pool,
                       const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);
        ~DescriptorSets();

        DescriptorSets(const DescriptorSets&) = delete;
        DescriptorSets& operator=(const DescriptorSets&) = delete;

        DescriptorSets(DescriptorSets&&) noexcept;
        DescriptorSets& operator=(DescriptorSets&&) noexcept;

        uint32_t count() const noexcept;
        const VkDescriptorSet& operator[](uint32_t index) const noexcept;
};

class Image final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkImage m_image = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Image* from) noexcept;

public:
        Image();
        Image(VkDevice device, const VkImageCreateInfo& create_info);
        ~Image();

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        Image(Image&&) noexcept;
        Image& operator=(Image&&) noexcept;

        operator VkImage() const noexcept;
};

class Sampler final
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Sampler* from) noexcept;

public:
        Sampler();
        Sampler(VkDevice device, const VkSamplerCreateInfo& create_info);
        ~Sampler();

        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;

        Sampler(Sampler&&) noexcept;
        Sampler& operator=(Sampler&&) noexcept;

        operator VkSampler() const noexcept;
};
}
