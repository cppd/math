/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "object_handles.h"

#include "error.h"
#include "extensions.h"

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#define MOVE_1(p0)                         \
        do                                 \
        {                                  \
                p0 = from->p0;             \
                from->p0 = VK_NULL_HANDLE; \
        } while (false)

#define MOVE_2(p0, p1)                     \
        do                                 \
        {                                  \
                p0 = from->p0;             \
                p1 = from->p1;             \
                from->p0 = VK_NULL_HANDLE; \
                from->p1 = VK_NULL_HANDLE; \
        } while (false)

#define MOVE_3(p0, p1, p2)                 \
        do                                 \
        {                                  \
                p0 = from->p0;             \
                p1 = from->p1;             \
                p2 = from->p2;             \
                from->p0 = VK_NULL_HANDLE; \
                from->p1 = VK_NULL_HANDLE; \
                from->p2 = VK_NULL_HANDLE; \
        } while (false)

namespace ns::vulkan::handle
{
void Instance::destroy() noexcept
{
        if (instance_ != VK_NULL_HANDLE)
        {
                vkDestroyInstance(instance_, nullptr);
        }
}

void Instance::move(Instance* const from) noexcept
{
        MOVE_1(instance_);
}

Instance::Instance(const VkInstanceCreateInfo& create_info)
{
        VULKAN_CHECK(vkCreateInstance(&create_info, nullptr, &instance_));
}

//

void DebugUtilsMessengerEXT::destroy() noexcept
{
        if (messenger_ != VK_NULL_HANDLE)
        {
                ASSERT(instance_ != VK_NULL_HANDLE);
                vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, nullptr);
        }
}

void DebugUtilsMessengerEXT::move(DebugUtilsMessengerEXT* const from) noexcept
{
        MOVE_2(instance_, messenger_);
}

DebugUtilsMessengerEXT::DebugUtilsMessengerEXT(
        const VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT& create_info)
        : instance_(instance)
{
        VULKAN_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &messenger_));
}

//

void Device::destroy() noexcept
{
        if (device_ != VK_NULL_HANDLE)
        {
                vkDestroyDevice(device_, nullptr);
        }
}

void Device::move(Device* const from) noexcept
{
        MOVE_1(device_);
}

Device::Device(const VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info)
{
        VULKAN_CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device_));
}

//

void SurfaceKHR::destroy() noexcept
{
        if (surface_ != VK_NULL_HANDLE)
        {
                ASSERT(instance_ != VK_NULL_HANDLE);
                vkDestroySurfaceKHR(instance_, surface_, nullptr);
        }
}

void SurfaceKHR::move(SurfaceKHR* const from) noexcept
{
        MOVE_2(instance_, surface_);
}

SurfaceKHR::SurfaceKHR(const VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : instance_(instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for VkSurfaceKHR creation");
        }
        surface_ = create_surface(instance);
        if (surface_ == VK_NULL_HANDLE)
        {
                error("VkSurfaceKHR not created");
        }
}

//

void SwapchainKHR::destroy() noexcept
{
        if (swapchain_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        }
}

void SwapchainKHR::move(SwapchainKHR* const from) noexcept
{
        MOVE_2(device_, swapchain_);
}

SwapchainKHR::SwapchainKHR(const VkDevice device, const VkSwapchainCreateInfoKHR& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain_));
}

//

void ShaderModule::destroy() noexcept
{
        if (shader_module_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyShaderModule(device_, shader_module_, nullptr);
        }
}

void ShaderModule::move(ShaderModule* const from) noexcept
{
        MOVE_2(device_, shader_module_);
}

ShaderModule::ShaderModule(const VkDevice device, const std::span<const std::uint32_t> code)
        : device_(device)
{
        static_assert(sizeof(std::uint32_t) == 4);

        if (code.empty())
        {
                error("Shader code size must be greater than 0");
        }

        VkShaderModuleCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size_bytes();
        info.pCode = code.data();
        VULKAN_CHECK(vkCreateShaderModule(device, &info, nullptr, &shader_module_));
}

//

void RenderPass::destroy() noexcept
{
        if (render_pass_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyRenderPass(device_, render_pass_, nullptr);
        }
}

void RenderPass::move(RenderPass* const from) noexcept
{
        MOVE_2(device_, render_pass_);
}

RenderPass::RenderPass(const VkDevice device, const VkRenderPassCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateRenderPass(device, &create_info, nullptr, &render_pass_));
}

//

void PipelineLayout::destroy() noexcept
{
        if (pipeline_layout_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        }
}

void PipelineLayout::move(PipelineLayout* const from) noexcept
{
        MOVE_2(device_, pipeline_layout_);
}

PipelineLayout::PipelineLayout(const VkDevice device, const VkPipelineLayoutCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreatePipelineLayout(device, &create_info, nullptr, &pipeline_layout_));
}

//

void Pipeline::destroy() noexcept
{
        if (pipeline_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyPipeline(device_, pipeline_, nullptr);
        }
}

void Pipeline::move(Pipeline* const from) noexcept
{
        MOVE_2(device_, pipeline_);
}

Pipeline::Pipeline(const VkDevice device, const VkGraphicsPipelineCreateInfo& create_info)
        : device_(device)
{
        static constexpr VkPipelineCache PIPELINE_CACHE = VK_NULL_HANDLE;

        VULKAN_CHECK(vkCreateGraphicsPipelines(device, PIPELINE_CACHE, 1, &create_info, nullptr, &pipeline_));
}

Pipeline::Pipeline(const VkDevice device, const VkComputePipelineCreateInfo& create_info)
        : device_(device)
{
        static constexpr VkPipelineCache PIPELINE_CACHE = VK_NULL_HANDLE;

        VULKAN_CHECK(vkCreateComputePipelines(device, PIPELINE_CACHE, 1, &create_info, nullptr, &pipeline_));
}

Pipeline::Pipeline(const VkDevice device, const VkRayTracingPipelineCreateInfoKHR& create_info)
        : device_(device)
{
        static constexpr VkDeferredOperationKHR DEFERRED_OPERATION = VK_NULL_HANDLE;
        static constexpr VkPipelineCache PIPELINE_CACHE = VK_NULL_HANDLE;

        VULKAN_CHECK(vkCreateRayTracingPipelinesKHR(
                device, DEFERRED_OPERATION, PIPELINE_CACHE, 1, &create_info, nullptr, &pipeline_));
}

//

void Framebuffer::destroy() noexcept
{
        if (framebuffer_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyFramebuffer(device_, framebuffer_, nullptr);
        }
}

void Framebuffer::move(Framebuffer* const from) noexcept
{
        MOVE_2(device_, framebuffer_);
}

Framebuffer::Framebuffer(const VkDevice device, const VkFramebufferCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateFramebuffer(device, &create_info, nullptr, &framebuffer_));
}

//

void CommandPool::destroy() noexcept
{
        if (command_pool_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyCommandPool(device_, command_pool_, nullptr);
        }
}

void CommandPool::move(CommandPool* const from) noexcept
{
        MOVE_2(device_, command_pool_);
}

CommandPool::CommandPool(const VkDevice device, const VkCommandPoolCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateCommandPool(device, &create_info, nullptr, &command_pool_));
}

//

void Semaphore::destroy() noexcept
{
        if (semaphore_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroySemaphore(device_, semaphore_, nullptr);
        }
}

void Semaphore::move(Semaphore* const from) noexcept
{
        MOVE_2(device_, semaphore_);
}

Semaphore::Semaphore(const VkDevice device)
        : device_(device)
{
        VkSemaphoreCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VULKAN_CHECK(vkCreateSemaphore(device, &info, nullptr, &semaphore_));
}

//

void Fence::destroy() noexcept
{
        if (fence_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyFence(device_, fence_, nullptr);
        }
}

void Fence::move(Fence* const from) noexcept
{
        MOVE_2(device_, fence_);
}

Fence::Fence(const VkDevice device, const bool signaled)
        : device_(device)
{
        VkFenceCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled)
        {
                info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        VULKAN_CHECK(vkCreateFence(device, &info, nullptr, &fence_));
}

//

void Buffer::destroy() noexcept
{
        if (buffer_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyBuffer(device_, buffer_, nullptr);
        }
}

void Buffer::move(Buffer* const from) noexcept
{
        MOVE_2(device_, buffer_);
}

Buffer::Buffer(const VkDevice device, const VkBufferCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateBuffer(device, &create_info, nullptr, &buffer_));
}

//

void DeviceMemory::destroy() noexcept
{
        if (device_memory_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkFreeMemory(device_, device_memory_, nullptr);
        }
}

void DeviceMemory::move(DeviceMemory* const from) noexcept
{
        MOVE_2(device_, device_memory_);
}

DeviceMemory::DeviceMemory(const VkDevice device, const VkMemoryAllocateInfo& allocate_info)
        : device_(device)
{
        VULKAN_CHECK(vkAllocateMemory(device, &allocate_info, nullptr, &device_memory_));
}

//

void CommandBuffer::destroy() noexcept
{
        if (command_buffer_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(command_pool_ != VK_NULL_HANDLE);
                vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer_);
        }
}

void CommandBuffer::move(CommandBuffer* const from) noexcept
{
        MOVE_3(device_, command_pool_, command_buffer_);
}

CommandBuffer::CommandBuffer(const VkDevice device, const VkCommandPool command_pool)
        : device_(device),
          command_pool_(command_pool)
{
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = 1;
        VULKAN_CHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer_));
}

//

void CommandBuffers::destroy() noexcept
{
        if (!command_buffers_.empty())
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(command_pool_ != VK_NULL_HANDLE);
                vkFreeCommandBuffers(device_, command_pool_, command_buffers_.size(), command_buffers_.data());
        }
}

void CommandBuffers::move(CommandBuffers* const from) noexcept
{
        MOVE_2(device_, command_pool_);
        command_buffers_ = std::move(from->command_buffers_);
        from->command_buffers_ = {};
}

CommandBuffers::CommandBuffers(const VkDevice device, const VkCommandPool command_pool, const std::uint32_t count)
        : device_(device),
          command_pool_(command_pool),
          command_buffers_(count)
{
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = count;
        VULKAN_CHECK(vkAllocateCommandBuffers(device, &allocate_info, command_buffers_.data()));
}

//

void DescriptorSetLayout::destroy() noexcept
{
        if (descriptor_set_layout_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
        }
}

void DescriptorSetLayout::move(DescriptorSetLayout* const from) noexcept
{
        MOVE_2(device_, descriptor_set_layout_);
}

DescriptorSetLayout::DescriptorSetLayout(const VkDevice device, const VkDescriptorSetLayoutCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptor_set_layout_));
}

//

void DescriptorPool::destroy() noexcept
{
        if (descriptor_pool_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
        }
}

void DescriptorPool::move(DescriptorPool* const from) noexcept
{
        MOVE_2(device_, descriptor_pool_);
}

DescriptorPool::DescriptorPool(const VkDevice device, const VkDescriptorPoolCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateDescriptorPool(device, &create_info, nullptr, &descriptor_pool_));
}

//

void DescriptorSet::destroy() noexcept
{
        if (descriptor_set_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(descriptor_pool_ != VK_NULL_HANDLE);
                VULKAN_CHECK(vkFreeDescriptorSets(device_, descriptor_pool_, 1, &descriptor_set_));
        }
}

void DescriptorSet::move(DescriptorSet* const from) noexcept
{
        MOVE_3(device_, descriptor_pool_, descriptor_set_);
}

DescriptorSet::DescriptorSet(
        const VkDevice device,
        const VkDescriptorPool descriptor_pool,
        const VkDescriptorSetLayout descriptor_set_layout)
        : device_(device),
          descriptor_pool_(descriptor_pool)
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(descriptor_pool != VK_NULL_HANDLE);
        ASSERT(descriptor_set_layout != VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &descriptor_set_layout;
        VULKAN_CHECK(vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set_));
}

//

void DescriptorSets::destroy() noexcept
{
        if (!descriptor_sets_.empty())
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(descriptor_pool_ != VK_NULL_HANDLE);
                VULKAN_CHECK(vkFreeDescriptorSets(
                        device_, descriptor_pool_, descriptor_sets_.size(), descriptor_sets_.data()));
        }
}

void DescriptorSets::move(DescriptorSets* const from) noexcept
{
        MOVE_2(device_, descriptor_pool_);
        descriptor_sets_ = std::move(from->descriptor_sets_);
        from->descriptor_sets_ = {};
}

DescriptorSets::DescriptorSets(
        const VkDevice device,
        const VkDescriptorPool descriptor_pool,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
        : device_(device),
          descriptor_pool_(descriptor_pool),
          descriptor_sets_(descriptor_set_layouts.size())
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(descriptor_pool != VK_NULL_HANDLE);
        ASSERT(!descriptor_set_layouts.empty());
        ASSERT(std::ranges::all_of(
                descriptor_set_layouts,
                [](const VkDescriptorSetLayout descriptor_set_layout)
                {
                        return descriptor_set_layout != VK_NULL_HANDLE;
                }));

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = descriptor_set_layouts.size();
        allocate_info.pSetLayouts = descriptor_set_layouts.data();
        VULKAN_CHECK(vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets_.data()));
}

//

void Image::destroy() noexcept
{
        if (image_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyImage(device_, image_, nullptr);
        }
}

void Image::move(Image* const from) noexcept
{
        MOVE_2(device_, image_);
}

Image::Image(const VkDevice device, const VkImageCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateImage(device, &create_info, nullptr, &image_));
}

//

void ImageView::destroy() noexcept
{
        if (image_view_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyImageView(device_, image_view_, nullptr);
        }
}

void ImageView::move(ImageView* const from) noexcept
{
        MOVE_2(device_, image_view_);
}

ImageView::ImageView(const VkDevice device, const VkImageViewCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateImageView(device, &create_info, nullptr, &image_view_));
}

//

void Sampler::destroy() noexcept
{
        if (sampler_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroySampler(device_, sampler_, nullptr);
        }
}

void Sampler::move(Sampler* const from) noexcept
{
        MOVE_2(device_, sampler_);
}

Sampler::Sampler(const VkDevice device, const VkSamplerCreateInfo& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateSampler(device, &create_info, nullptr, &sampler_));
}

//

void AccelerationStructureKHR::destroy() noexcept
{
        if (acceleration_structure_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                vkDestroyAccelerationStructureKHR(device_, acceleration_structure_, nullptr);
        }
}

void AccelerationStructureKHR::move(AccelerationStructureKHR* const from) noexcept
{
        MOVE_2(device_, acceleration_structure_);
}

AccelerationStructureKHR::AccelerationStructureKHR(
        const VkDevice device,
        const VkAccelerationStructureCreateInfoKHR& create_info)
        : device_(device)
{
        VULKAN_CHECK(vkCreateAccelerationStructureKHR(device, &create_info, nullptr, &acceleration_structure_));
}
}
