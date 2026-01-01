/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <functional>
#include <span>
#include <vector>

#define VULKAN_HANDLE_SPECIAL_FUNCTIONS(name)  \
        name() = default;                      \
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

#define VULKAN_HANDLE_INSTANCE_FUNCTIONS(name, v) \
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(name)     \
        operator Vk##name() const& noexcept       \
        {                                         \
                return (v);                       \
        }                                         \
        operator Vk##name() const&& = delete;     \
        VkInstance instance() const noexcept      \
        {                                         \
                return instance_;                 \
        }

#define VULKAN_HANDLE_DEVICE_FUNCTIONS(name, v) \
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(name)   \
        operator Vk##name() const& noexcept     \
        {                                       \
                return (v);                     \
        }                                       \
        operator Vk##name() const&& = delete;   \
        VkDevice device() const noexcept        \
        {                                       \
                return device_;                 \
        }

#define VULKAN_HANDLE_DEVICE_VECTOR_FUNCTIONS(name, v)                       \
        VULKAN_HANDLE_SPECIAL_FUNCTIONS(name)                                \
        decltype(auto) operator[](const std::uint32_t index) const& noexcept \
        {                                                                    \
                ASSERT(index < (v).size());                                  \
                return (v)[index];                                           \
        }                                                                    \
        auto operator[](const std::uint32_t) const&& = delete;               \
        std::uint32_t count() const noexcept                                 \
        {                                                                    \
                return (v).size();                                           \
        }                                                                    \
        VkDevice device() const noexcept                                     \
        {                                                                    \
                return device_;                                              \
        }

namespace ns::vulkan::handle
{
class Instance final
{
        VkInstance instance_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        VULKAN_HANDLE_INSTANCE_FUNCTIONS(Instance, instance_)

        explicit Instance(const VkInstanceCreateInfo& create_info);
};

class DebugUtilsMessengerEXT final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DebugUtilsMessengerEXT* from) noexcept;

public:
        VULKAN_HANDLE_INSTANCE_FUNCTIONS(DebugUtilsMessengerEXT, messenger_)

        DebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT& create_info);
};

class SurfaceKHR final
{
        VkInstance instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SurfaceKHR* from) noexcept;

public:
        VULKAN_HANDLE_INSTANCE_FUNCTIONS(SurfaceKHR, surface_)

        SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
};

class Device final
{
        VkDevice device_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Device* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Device, device_)

        Device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info);
};

class SwapchainKHR final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SwapchainKHR* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(SwapchainKHR, swapchain_)

        SwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info);
};

class ShaderModule final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkShaderModule shader_module_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(ShaderModule, shader_module_)

        ShaderModule(VkDevice device, std::span<const std::uint32_t> code);
};

class RenderPass final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkRenderPass render_pass_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(RenderPass* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(RenderPass, render_pass_)

        RenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info);
};

class PipelineLayout final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(PipelineLayout* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(PipelineLayout, pipeline_layout_)

        PipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info);
};

class Pipeline final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Pipeline* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Pipeline, pipeline_)

        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info);
        Pipeline(VkDevice device, const VkComputePipelineCreateInfo& create_info);
        Pipeline(VkDevice device, const VkRayTracingPipelineCreateInfoKHR& create_info);
};

class Framebuffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFramebuffer framebuffer_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Framebuffer* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Framebuffer, framebuffer_)

        Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info);
};

class CommandPool final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(CommandPool* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(CommandPool, command_pool_)

        CommandPool(VkDevice device, const VkCommandPoolCreateInfo& create_info);
};

class Semaphore final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSemaphore semaphore_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Semaphore* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Semaphore, semaphore_)

        explicit Semaphore(VkDevice device);
};

class Fence final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkFence fence_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Fence* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Fence, fence_)

        Fence(VkDevice device, bool signaled);
};

class Buffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkBuffer buffer_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Buffer* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Buffer, buffer_)

        Buffer(VkDevice device, const VkBufferCreateInfo& create_info);
};

class DeviceMemory final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDeviceMemory device_memory_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DeviceMemory* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(DeviceMemory, device_memory_)

        DeviceMemory(VkDevice device, const VkMemoryAllocateInfo& allocate_info);
};

class CommandBuffer final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(CommandBuffer* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(CommandBuffer, command_buffer_)

        CommandBuffer(VkDevice device, VkCommandPool command_pool);
};

class CommandBuffers final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> command_buffers_;

        void destroy() noexcept;
        void move(CommandBuffers* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_VECTOR_FUNCTIONS(CommandBuffers, command_buffers_)

        CommandBuffers(VkDevice device, VkCommandPool command_pool, std::uint32_t count);
};

class DescriptorSetLayout final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorSetLayout* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(DescriptorSetLayout, descriptor_set_layout_)

        DescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& create_info);
};

class DescriptorPool final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorPool* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(DescriptorPool, descriptor_pool_)

        DescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& create_info);
};

class DescriptorSet final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DescriptorSet* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(DescriptorSet, descriptor_set_)

        DescriptorSet(VkDevice device, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);
};

class DescriptorSets final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptor_sets_;

        void destroy() noexcept;
        void move(DescriptorSets* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_VECTOR_FUNCTIONS(DescriptorSets, descriptor_sets_)

        DescriptorSets(
                VkDevice device,
                VkDescriptorPool descriptor_pool,
                const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts);
};

class Image final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkImage image_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Image* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Image, image_)

        Image(VkDevice device, const VkImageCreateInfo& create_info);
};

class ImageView final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkImageView image_view_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ImageView* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(ImageView, image_view_)

        ImageView(VkDevice device, const VkImageViewCreateInfo& create_info);
};

class Sampler final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkSampler sampler_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Sampler* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(Sampler, sampler_)

        Sampler(VkDevice device, const VkSamplerCreateInfo& create_info);
};

class AccelerationStructureKHR final
{
        VkDevice device_ = VK_NULL_HANDLE;
        VkAccelerationStructureKHR acceleration_structure_ = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(AccelerationStructureKHR* from) noexcept;

public:
        VULKAN_HANDLE_DEVICE_FUNCTIONS(AccelerationStructureKHR, acceleration_structure_)

        AccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR& create_info);
};
}
