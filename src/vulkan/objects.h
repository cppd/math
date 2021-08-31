/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/type/limit.h>

#include <functional>
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
class InstanceHandle final
{
        VkInstance instance_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(InstanceHandle* from) noexcept;

public:
        explicit InstanceHandle(const VkInstanceCreateInfo& create_info);
        ~InstanceHandle();

        InstanceHandle(const InstanceHandle&) = delete;
        InstanceHandle& operator=(const InstanceHandle&) = delete;

        InstanceHandle(InstanceHandle&&) noexcept;
        InstanceHandle& operator=(InstanceHandle&&) noexcept;

        operator VkInstance() const& noexcept;
        operator VkInstance() const&& noexcept = delete;
};

class Instance final
{
        InstanceHandle instance_;
        bool validation_layers_enabled_;

public:
        explicit Instance(const VkInstanceCreateInfo& create_info)
                : instance_(create_info), validation_layers_enabled_(create_info.enabledLayerCount > 0)
        {
        }

        operator VkInstance() const& noexcept
        {
                return instance_;
        }

        operator VkInstance() const&& noexcept = delete;

        bool validation_layers_enabled() const noexcept
        {
                return validation_layers_enabled_;
        }
};

class DebugReportCallback final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT callback_ = VK_NULL_HANDLE;

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

        operator VkDebugReportCallbackEXT() const& noexcept;
        operator VkDebugReportCallbackEXT() const&& noexcept = delete;
};

class DeviceHandle final
{
        VkDevice device_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DeviceHandle* from) noexcept;

public:
        DeviceHandle();
        DeviceHandle(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info);
        ~DeviceHandle();

        DeviceHandle(const DeviceHandle&) = delete;
        DeviceHandle& operator=(const DeviceHandle&) = delete;

        DeviceHandle(DeviceHandle&&) noexcept;
        DeviceHandle& operator=(DeviceHandle&&) noexcept;

        operator VkDevice() const& noexcept;
        operator VkDevice() const&& noexcept = delete;
};

class Queue final
{
        VkQueue queue_ = VK_NULL_HANDLE;
        uint32_t family_index_ = limits<uint32_t>::max();

public:
        Queue() = default;
        Queue(uint32_t family_index, VkQueue queue) : queue_(queue), family_index_(family_index)
        {
        }
        operator VkQueue() const& noexcept
        {
                return queue_;
        }
        operator VkQueue() const&& noexcept = delete;
        uint32_t family_index() const noexcept
        {
                return family_index_;
        }
};

struct DeviceFeatures final
{
        VkPhysicalDeviceFeatures features_10{};
        VkPhysicalDeviceVulkan11Features features_11{};
        VkPhysicalDeviceVulkan12Features features_12{};
};

struct DeviceProperties final
{
        VkPhysicalDeviceProperties properties_10;
        VkPhysicalDeviceVulkan11Properties properties_11;
        VkPhysicalDeviceVulkan12Properties properties_12;
};

class Device final
{
        DeviceHandle device_;
        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        const DeviceProperties* physical_device_properties_ = nullptr;
        DeviceFeatures features_;
        std::unordered_map<uint32_t, std::vector<VkQueue>> queues_;

public:
        Device() = default;

        Device(VkPhysicalDevice physical_device,
               const DeviceProperties* physical_device_properties,
               const VkDeviceCreateInfo& create_info);

        operator VkDevice() const& noexcept
        {
                return device_;
        }
        operator VkDevice() const&& noexcept = delete;

        VkPhysicalDevice physical_device() const noexcept
        {
                return physical_device_;
        }

        const DeviceFeatures& features() const noexcept
        {
                return features_;
        }

        const DeviceProperties& properties() const noexcept
        {
                return *physical_device_properties_;
        }

        Queue queue(uint32_t family_index, uint32_t queue_index) const;
};

class SurfaceKHR final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;

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

        operator VkSurfaceKHR() const& noexcept;
        operator VkSurfaceKHR() const&& noexcept = delete;
};

class SwapchainKHR final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

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

        operator VkSwapchainKHR() const& noexcept;
        operator VkSwapchainKHR() const&& noexcept = delete;
};

class ImageView final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkImageView image_view_ = VK_NULL_HANDLE;

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

        operator VkImageView() const& noexcept;
        operator VkImageView() const&& noexcept = delete;
};

class ShaderModule final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkShaderModule shader_module_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        ShaderModule();
        ShaderModule(VkDevice device, const std::span<const uint32_t>& code);
        ~ShaderModule();

        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;

        ShaderModule(ShaderModule&&) noexcept;
        ShaderModule& operator=(ShaderModule&&) noexcept;

        operator VkShaderModule() const& noexcept;
        operator VkShaderModule() const&& noexcept = delete;
};

class RenderPass final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkRenderPass render_pass_ = VK_NULL_HANDLE;

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

        operator VkRenderPass() const& noexcept;
        operator VkRenderPass() const&& noexcept = delete;
};

class PipelineLayout final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;

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

        operator VkPipelineLayout() const& noexcept;
        operator VkPipelineLayout() const&& noexcept = delete;
};

class Pipeline final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

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

        operator VkPipeline() const& noexcept;
        operator VkPipeline() const&& noexcept = delete;
};

class Framebuffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

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

        operator VkFramebuffer() const& noexcept;
        operator VkFramebuffer() const&& noexcept = delete;
};

class CommandPool final
{
        static constexpr uint32_t NULL_FAMILY_INDEX = limits<uint32_t>::max();

        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        uint32_t family_index_ = NULL_FAMILY_INDEX;

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

        operator VkCommandPool() const& noexcept;
        operator VkCommandPool() const&& noexcept = delete;

        uint32_t family_index() const noexcept;
};

class Semaphore final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSemaphore semaphore_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Semaphore* from) noexcept;

public:
        Semaphore();
        explicit Semaphore(VkDevice device);
        ~Semaphore();

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&&) noexcept;
        Semaphore& operator=(Semaphore&&) noexcept;

        operator VkSemaphore() const& noexcept;
        operator VkSemaphore() const&& noexcept = delete;
};

class Fence final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFence fence_ = VK_NULL_HANDLE;

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

        operator VkFence() const& noexcept;
        operator VkFence() const&& noexcept = delete;
};

class BufferHandle final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkBuffer buffer_ = VK_NULL_HANDLE;

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

        operator VkBuffer() const& noexcept
        {
                return buffer_;
        }
        operator VkBuffer() const&& noexcept = delete;
        VkDevice device() const noexcept
        {
                return device_;
        }
};

class Buffer final
{
        BufferHandle buffer_;
        VkDeviceSize size_;
        VkBufferUsageFlags usage_;

public:
        Buffer() = default;

        Buffer(VkDevice device, const VkBufferCreateInfo& create_info)
                : buffer_(device, create_info), size_(create_info.size), usage_(create_info.usage)
        {
        }

        operator VkBuffer() const& noexcept
        {
                return buffer_;
        }
        operator VkBuffer() const&& noexcept = delete;
        VkDevice device() const noexcept
        {
                return buffer_.device();
        }
        VkDeviceSize size() const noexcept
        {
                return size_;
        }
        bool has_usage(VkBufferUsageFlagBits flag) const noexcept
        {
                return (usage_ & flag) == flag;
        }
};

class DeviceMemory final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDeviceMemory device_memory_ = VK_NULL_HANDLE;

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

        operator VkDeviceMemory() const& noexcept
        {
                return device_memory_;
        }
        operator VkDeviceMemory() const&& noexcept = delete;
        VkDevice device() const noexcept
        {
                return device_;
        }
};

class CommandBuffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;

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

        operator VkCommandBuffer() const& noexcept;
        operator VkCommandBuffer() const&& noexcept = delete;
};

class CommandBuffers final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> command_buffers_;

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
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

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

        operator VkDescriptorSetLayout() const& noexcept;
        operator VkDescriptorSetLayout() const&& noexcept = delete;
};

class DescriptorPool final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

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

        operator VkDescriptorPool() const& noexcept;
        operator VkDescriptorPool() const&& noexcept = delete;
};

class DescriptorSet final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

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

        operator VkDescriptorSet() const& noexcept;
        operator VkDescriptorSet() const&& noexcept = delete;
};

class DescriptorSets final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptor_sets_;

        void destroy() noexcept;
        void move(DescriptorSets* from) noexcept;

public:
        DescriptorSets();
        DescriptorSets(
                VkDevice device,
                VkDescriptorPool descriptor_pool,
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
        VkDevice device_ = VK_NULL_HANDLE;
        VkImage image_ = VK_NULL_HANDLE;

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

        operator VkImage() const& noexcept;
        operator VkImage() const&& noexcept = delete;
};

class Sampler final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSampler sampler_ = VK_NULL_HANDLE;

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

        operator VkSampler() const& noexcept;
        operator VkSampler() const&& noexcept = delete;
};
}
