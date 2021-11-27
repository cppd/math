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

#include <cstdint>
#include <functional>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

#define VULKAN_HANDLE_SPECIAL_FUNCTIONS(name)  \
        name(const name&) = delete;            \
        name& operator=(const name&) = delete; \
        ~name()                                \
        {                                      \
                destroy();                     \
        }                                      \
        name(name&& from) noexcept             \
        {                                      \
                move(&from);                   \
        }                                      \
        name& operator=(name&& from) noexcept  \
        {                                      \
                if (this != &from)             \
                {                              \
                        destroy();             \
                        move(&from);           \
                }                              \
                return *this;                  \
        }

namespace ns::vulkan::handle
{
class Instance final
{
        VkInstance instance_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Instance)

        explicit Instance(const VkInstanceCreateInfo& create_info);

        operator VkInstance() const& noexcept
        {
                return instance_;
        }
        operator VkInstance() const&& noexcept = delete;
};

class DebugReportCallback final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT callback_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DebugReportCallback* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DebugReportCallback)

        DebugReportCallback() = default;
        DebugReportCallback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT& create_info);

        operator VkDebugReportCallbackEXT() const& noexcept
        {
                return callback_;
        }
        operator VkDebugReportCallbackEXT() const&& noexcept = delete;
};

class Device final
{
        VkDevice device_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Device* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Device)

        Device() = default;
        Device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info);

        operator VkDevice() const& noexcept
        {
                return device_;
        }
        operator VkDevice() const&& noexcept = delete;
};

class SurfaceKHR final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SurfaceKHR* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(SurfaceKHR)

        SurfaceKHR() = default;
        SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);

        operator VkSurfaceKHR() const& noexcept
        {
                return surface_;
        }
        operator VkSurfaceKHR() const&& noexcept = delete;
};

class SwapchainKHR final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SwapchainKHR* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(SwapchainKHR)

        SwapchainKHR() = default;
        SwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info);

        operator VkSwapchainKHR() const& noexcept
        {
                return swapchain_;
        }
        operator VkSwapchainKHR() const&& noexcept = delete;
};

class ShaderModule final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkShaderModule shader_module_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(ShaderModule)

        ShaderModule() = default;
        ShaderModule(VkDevice device, const std::span<const std::uint32_t>& code);

        operator VkShaderModule() const& noexcept
        {
                return shader_module_;
        }
        operator VkShaderModule() const&& noexcept = delete;
};

class RenderPass final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkRenderPass render_pass_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(RenderPass* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(RenderPass)

        RenderPass() = default;
        RenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info);

        operator VkRenderPass() const& noexcept
        {
                return render_pass_;
        }
        operator VkRenderPass() const&& noexcept = delete;
};

class PipelineLayout final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(PipelineLayout* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(PipelineLayout)

        PipelineLayout() = default;
        PipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info);

        operator VkPipelineLayout() const& noexcept
        {
                return pipeline_layout_;
        }
        operator VkPipelineLayout() const&& noexcept = delete;
};

class Pipeline final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Pipeline* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Pipeline)

        Pipeline() = default;
        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info);
        Pipeline(VkDevice device, const VkComputePipelineCreateInfo& create_info);

        operator VkPipeline() const& noexcept
        {
                return pipeline_;
        }
        operator VkPipeline() const&& noexcept = delete;
};

class Framebuffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Framebuffer* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Framebuffer)

        Framebuffer() = default;
        Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info);

        operator VkFramebuffer() const& noexcept
        {
                return framebuffer_;
        }
        operator VkFramebuffer() const&& noexcept = delete;
};

class CommandPool final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(CommandPool* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(CommandPool)

        CommandPool() = default;
        CommandPool(VkDevice device, const VkCommandPoolCreateInfo& create_info);

        operator VkCommandPool() const& noexcept
        {
                return command_pool_;
        }
        operator VkCommandPool() const&& noexcept = delete;
};

class Semaphore final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSemaphore semaphore_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Semaphore* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Semaphore)

        Semaphore() = default;
        explicit Semaphore(VkDevice device);

        operator VkSemaphore() const& noexcept
        {
                return semaphore_;
        }
        operator VkSemaphore() const&& noexcept = delete;
};

class Fence final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFence fence_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Fence* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Fence)

        Fence() = default;
        Fence(VkDevice device, bool signaled);

        operator VkFence() const& noexcept
        {
                return fence_;
        }
        operator VkFence() const&& noexcept = delete;
};

class Buffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkBuffer buffer_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Buffer* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Buffer)

        Buffer() = default;
        Buffer(VkDevice device, const VkBufferCreateInfo& create_info);

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

class DeviceMemory final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDeviceMemory device_memory_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DeviceMemory* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DeviceMemory)

        DeviceMemory() = default;
        DeviceMemory(VkDevice device, const VkMemoryAllocateInfo& allocate_info);

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
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(CommandBuffer)

        CommandBuffer() = default;
        CommandBuffer(VkDevice device, VkCommandPool command_pool);

        operator VkCommandBuffer() const& noexcept
        {
                return command_buffer_;
        }
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
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(CommandBuffers)

        CommandBuffers() = default;
        CommandBuffers(VkDevice device, VkCommandPool command_pool, std::uint32_t count);

        const VkCommandBuffer& operator[](std::uint32_t index) const noexcept;
        std::uint32_t count() const noexcept;
};

class DescriptorSetLayout final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorSetLayout* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DescriptorSetLayout)

        DescriptorSetLayout() = default;
        DescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& create_info);

        operator VkDescriptorSetLayout() const& noexcept
        {
                return descriptor_set_layout_;
        }
        operator VkDescriptorSetLayout() const&& noexcept = delete;
};

class DescriptorPool final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorPool* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DescriptorPool)

        DescriptorPool() = default;
        DescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& create_info);

        operator VkDescriptorPool() const& noexcept
        {
                return descriptor_pool_;
        }
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
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DescriptorSet)

        DescriptorSet() = default;
        DescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);

        operator VkDescriptorSet() const& noexcept
        {
                return descriptor_set_;
        }
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
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(DescriptorSets)

        DescriptorSets() = default;
        DescriptorSets(
                VkDevice device,
                VkDescriptorPool descriptor_pool,
                const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);

        const VkDescriptorSet& operator[](std::uint32_t index) const noexcept;
        std::uint32_t count() const noexcept;
};

class Image final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkImage image_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Image* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Image)

        Image() = default;
        Image(VkDevice device, const VkImageCreateInfo& create_info);

        operator VkImage() const& noexcept
        {
                return image_;
        }
        operator VkImage() const&& noexcept = delete;

        VkDevice device() const noexcept
        {
                return device_;
        }
};

class ImageView final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkImageView image_view_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ImageView* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(ImageView)

        ImageView() = default;
        ImageView(VkDevice device, const VkImageViewCreateInfo& create_info);

        operator VkImageView() const& noexcept
        {
                return image_view_;
        }
        operator VkImageView() const&& noexcept = delete;
};

class Sampler final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSampler sampler_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Sampler* from) noexcept;

public:
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(Sampler)

        Sampler() = default;
        Sampler(VkDevice device, const VkSamplerCreateInfo& create_info);

        operator VkSampler() const& noexcept
        {
                return sampler_;
        }
        operator VkSampler() const&& noexcept = delete;
};
}
