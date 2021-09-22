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

#include "objects.h"

#include "error.h"
#include "extensions.h"

#include <src/com/error.h>

#include <algorithm>

namespace ns::vulkan
{
void InstanceHandle::destroy() noexcept
{
        if (instance_ != VK_NULL_HANDLE)
        {
                vkDestroyInstance(instance_, nullptr);
        }
}

void InstanceHandle::move(InstanceHandle* from) noexcept
{
        instance_ = from->instance_;
        from->instance_ = VK_NULL_HANDLE;
}

InstanceHandle::InstanceHandle(const VkInstanceCreateInfo& create_info)
{
        VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateInstance", result);
        }

        ASSERT(instance_ != VK_NULL_HANDLE);
}

InstanceHandle::~InstanceHandle()
{
        destroy();
}

InstanceHandle::InstanceHandle(InstanceHandle&& from) noexcept
{
        move(&from);
}

InstanceHandle& InstanceHandle::operator=(InstanceHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void DebugReportCallback::destroy() noexcept
{
        if (callback_ != VK_NULL_HANDLE)
        {
                ASSERT(instance_ != VK_NULL_HANDLE);

                vkDestroyDebugReportCallbackEXT(instance_, callback_, nullptr);
        }
}

void DebugReportCallback::move(DebugReportCallback* from) noexcept
{
        instance_ = from->instance_;
        callback_ = from->callback_;
        from->instance_ = VK_NULL_HANDLE;
        from->callback_ = VK_NULL_HANDLE;
}

DebugReportCallback::DebugReportCallback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT& create_info)
{
        VkResult result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &callback_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDebugReportCallbackEXT", result);
        }

        instance_ = instance;

        ASSERT(callback_ != VK_NULL_HANDLE);
}

DebugReportCallback::~DebugReportCallback()
{
        destroy();
}

DebugReportCallback::DebugReportCallback(DebugReportCallback&& from) noexcept
{
        move(&from);
}

DebugReportCallback& DebugReportCallback::operator=(DebugReportCallback&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void DeviceHandle::destroy() noexcept
{
        if (device_ != VK_NULL_HANDLE)
        {
                vkDestroyDevice(device_, nullptr);
        }
}

void DeviceHandle::move(DeviceHandle* from) noexcept
{
        device_ = from->device_;
        from->device_ = VK_NULL_HANDLE;
}

DeviceHandle::DeviceHandle(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info)
{
        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &device_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDevice", result);
        }

        ASSERT(device_ != VK_NULL_HANDLE);
}

DeviceHandle::~DeviceHandle()
{
        destroy();
}

DeviceHandle::DeviceHandle(DeviceHandle&& from) noexcept
{
        move(&from);
}

DeviceHandle& DeviceHandle::operator=(DeviceHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void SurfaceKHR::move(SurfaceKHR* from) noexcept
{
        instance_ = from->instance_;
        surface_ = from->surface_;
        from->instance_ = VK_NULL_HANDLE;
        from->surface_ = VK_NULL_HANDLE;
}

SurfaceKHR::SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for VkSurfaceKHR creation");
        }

        surface_ = create_surface(instance);

        ASSERT(surface_ != VK_NULL_HANDLE);

        instance_ = instance;
}

SurfaceKHR::~SurfaceKHR()
{
        destroy();
}

SurfaceKHR::SurfaceKHR(SurfaceKHR&& from) noexcept
{
        move(&from);
}

SurfaceKHR& SurfaceKHR::operator=(SurfaceKHR&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void SwapchainKHR::move(SwapchainKHR* from) noexcept
{
        device_ = from->device_;
        swapchain_ = from->swapchain_;
        from->device_ = VK_NULL_HANDLE;
        from->swapchain_ = VK_NULL_HANDLE;
}

SwapchainKHR::SwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info)
{
        VkResult result = vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateSwapchainKHR", result);
        }

        ASSERT(swapchain_ != VK_NULL_HANDLE);

        device_ = device;
}

SwapchainKHR::~SwapchainKHR()
{
        destroy();
}

SwapchainKHR::SwapchainKHR(SwapchainKHR&& from) noexcept
{
        move(&from);
}

SwapchainKHR& SwapchainKHR::operator=(SwapchainKHR&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void ShaderModule::move(ShaderModule* from) noexcept
{
        device_ = from->device_;
        shader_module_ = from->shader_module_;
        from->device_ = VK_NULL_HANDLE;
        from->shader_module_ = VK_NULL_HANDLE;
}

ShaderModule::ShaderModule(VkDevice device, const std::span<const uint32_t>& code)
{
        static_assert(sizeof(uint32_t) == 4);

        if (code.empty())
        {
                error("Shader code size must be greater than 0");
        }

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size_bytes();
        create_info.pCode = code.data();

        //

        VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateShaderModule", result);
        }

        ASSERT(shader_module_ != VK_NULL_HANDLE);

        device_ = device;
}

ShaderModule::~ShaderModule()
{
        destroy();
}

ShaderModule::ShaderModule(ShaderModule&& from) noexcept
{
        move(&from);
}

ShaderModule& ShaderModule::operator=(ShaderModule&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void RenderPass::move(RenderPass* from) noexcept
{
        device_ = from->device_;
        render_pass_ = from->render_pass_;
        from->device_ = VK_NULL_HANDLE;
        from->render_pass_ = VK_NULL_HANDLE;
}

RenderPass::RenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info)
{
        VkResult result = vkCreateRenderPass(device, &create_info, nullptr, &render_pass_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateRenderPass", result);
        }

        ASSERT(render_pass_ != VK_NULL_HANDLE);

        device_ = device;
}

RenderPass::~RenderPass()
{
        destroy();
}

RenderPass::RenderPass(RenderPass&& from) noexcept
{
        move(&from);
}

RenderPass& RenderPass::operator=(RenderPass&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void PipelineLayout::move(PipelineLayout* from) noexcept
{
        device_ = from->device_;
        pipeline_layout_ = from->pipeline_layout_;
        from->device_ = VK_NULL_HANDLE;
        from->pipeline_layout_ = VK_NULL_HANDLE;
}

PipelineLayout::PipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info)
{
        VkResult result = vkCreatePipelineLayout(device, &create_info, nullptr, &pipeline_layout_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreatePipelineLayout", result);
        }

        ASSERT(pipeline_layout_ != VK_NULL_HANDLE);

        device_ = device;
}

PipelineLayout::~PipelineLayout()
{
        destroy();
}

PipelineLayout::PipelineLayout(PipelineLayout&& from) noexcept
{
        move(&from);
}

PipelineLayout& PipelineLayout::operator=(PipelineLayout&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void Pipeline::move(Pipeline* from) noexcept
{
        device_ = from->device_;
        pipeline_ = from->pipeline_;
        from->device_ = VK_NULL_HANDLE;
        from->pipeline_ = VK_NULL_HANDLE;
}

Pipeline::Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info)
{
        VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

        VkResult result = vkCreateGraphicsPipelines(device, pipeline_cache, 1, &create_info, nullptr, &pipeline_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateGraphicsPipelines", result);
        }

        ASSERT(pipeline_ != VK_NULL_HANDLE);

        device_ = device;
}

Pipeline::Pipeline(VkDevice device, const VkComputePipelineCreateInfo& create_info)
{
        VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

        VkResult result = vkCreateComputePipelines(device, pipeline_cache, 1, &create_info, nullptr, &pipeline_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateComputePipelines", result);
        }

        ASSERT(pipeline_ != VK_NULL_HANDLE);

        device_ = device;
}

Pipeline::~Pipeline()
{
        destroy();
}

Pipeline::Pipeline(Pipeline&& from) noexcept
{
        move(&from);
}

Pipeline& Pipeline::operator=(Pipeline&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void Framebuffer::move(Framebuffer* from) noexcept
{
        device_ = from->device_;
        framebuffer_ = from->framebuffer_;
        from->device_ = VK_NULL_HANDLE;
        from->framebuffer_ = VK_NULL_HANDLE;
}

Framebuffer::Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info)
{
        VkResult result = vkCreateFramebuffer(device, &create_info, nullptr, &framebuffer_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateFramebuffer", result);
        }

        ASSERT(framebuffer_ != VK_NULL_HANDLE);

        device_ = device;
}

Framebuffer::~Framebuffer()
{
        destroy();
}

Framebuffer::Framebuffer(Framebuffer&& from) noexcept
{
        move(&from);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void CommandPool::move(CommandPool* from) noexcept
{
        device_ = from->device_;
        command_pool_ = from->command_pool_;
        family_index_ = from->family_index_;
        from->device_ = VK_NULL_HANDLE;
        from->command_pool_ = VK_NULL_HANDLE;
        from->family_index_ = NULL_FAMILY_INDEX;
}

CommandPool::CommandPool(VkDevice device, const VkCommandPoolCreateInfo& create_info)
{
        VkResult result = vkCreateCommandPool(device, &create_info, nullptr, &command_pool_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateCommandPool", result);
        }

        ASSERT(command_pool_ != VK_NULL_HANDLE);

        device_ = device;
        family_index_ = create_info.queueFamilyIndex;
}

CommandPool::~CommandPool()
{
        destroy();
}

CommandPool::CommandPool(CommandPool&& from) noexcept
{
        move(&from);
}

CommandPool& CommandPool::operator=(CommandPool&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

uint32_t CommandPool::family_index() const noexcept
{
        return family_index_;
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

void Semaphore::move(Semaphore* from) noexcept
{
        device_ = from->device_;
        semaphore_ = from->semaphore_;
        from->device_ = VK_NULL_HANDLE;
        from->semaphore_ = VK_NULL_HANDLE;
}

Semaphore::Semaphore(VkDevice device)
{
        VkSemaphoreCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        //

        VkResult result = vkCreateSemaphore(device, &create_info, nullptr, &semaphore_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateSemaphore", result);
        }

        ASSERT(semaphore_ != VK_NULL_HANDLE);

        device_ = device;
}

Semaphore::~Semaphore()
{
        destroy();
}

Semaphore::Semaphore(Semaphore&& from) noexcept
{
        move(&from);
}

Semaphore& Semaphore::operator=(Semaphore&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void Fence::move(Fence* from) noexcept
{
        device_ = from->device_;
        fence_ = from->fence_;
        from->device_ = VK_NULL_HANDLE;
        from->fence_ = VK_NULL_HANDLE;
}

Fence::Fence(VkDevice device, bool signaled)
{
        VkFenceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled)
        {
                create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        //

        VkResult result = vkCreateFence(device, &create_info, nullptr, &fence_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateFence", result);
        }

        ASSERT(fence_ != VK_NULL_HANDLE);

        device_ = device;
}

Fence::~Fence()
{
        destroy();
}

Fence::Fence(Fence&& from) noexcept
{
        move(&from);
}

Fence& Fence::operator=(Fence&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void BufferHandle::destroy() noexcept
{
        if (buffer_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);

                vkDestroyBuffer(device_, buffer_, nullptr);
        }
}

void BufferHandle::move(BufferHandle* from) noexcept
{
        device_ = from->device_;
        buffer_ = from->buffer_;
        from->device_ = VK_NULL_HANDLE;
        from->buffer_ = VK_NULL_HANDLE;
}

BufferHandle::BufferHandle(VkDevice device, const VkBufferCreateInfo& create_info)
{
        VkResult result = vkCreateBuffer(device, &create_info, nullptr, &buffer_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateBuffer", result);
        }

        ASSERT(buffer_ != VK_NULL_HANDLE);

        device_ = device;
}

BufferHandle::~BufferHandle()
{
        destroy();
}

BufferHandle::BufferHandle(BufferHandle&& from) noexcept
{
        move(&from);
}

BufferHandle& BufferHandle::operator=(BufferHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void DeviceMemory::move(DeviceMemory* from) noexcept
{
        device_ = from->device_;
        device_memory_ = from->device_memory_;
        from->device_ = VK_NULL_HANDLE;
        from->device_memory_ = VK_NULL_HANDLE;
}

DeviceMemory::DeviceMemory(VkDevice device, const VkMemoryAllocateInfo& allocate_info)
{
        VkResult result = vkAllocateMemory(device, &allocate_info, nullptr, &device_memory_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAllocateMemory", result);
        }

        ASSERT(device_memory_ != VK_NULL_HANDLE);

        device_ = device;
}

DeviceMemory::~DeviceMemory()
{
        destroy();
}

DeviceMemory::DeviceMemory(DeviceMemory&& from) noexcept
{
        move(&from);
}

DeviceMemory& DeviceMemory::operator=(DeviceMemory&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void CommandBuffer::move(CommandBuffer* from) noexcept
{
        device_ = from->device_;
        command_pool_ = from->command_pool_;
        command_buffer_ = from->command_buffer_;
        from->device_ = VK_NULL_HANDLE;
        from->command_pool_ = VK_NULL_HANDLE;
        from->command_buffer_ = VK_NULL_HANDLE;
}

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool command_pool)
{
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = 1;

        //

        VkResult result = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAllocateCommandBuffers", result);
        }

        ASSERT(command_buffer_ != VK_NULL_HANDLE);

        device_ = device;
        command_pool_ = command_pool;
}

CommandBuffer::~CommandBuffer()
{
        destroy();
}

CommandBuffer::CommandBuffer(CommandBuffer&& from) noexcept
{
        move(&from);
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void CommandBuffers::move(CommandBuffers* from) noexcept
{
        device_ = from->device_;
        command_pool_ = from->command_pool_;
        command_buffers_ = std::move(from->command_buffers_);
        from->device_ = VK_NULL_HANDLE;
        from->command_pool_ = VK_NULL_HANDLE;
        from->command_buffers_ = std::vector<VkCommandBuffer>();
}

CommandBuffers::CommandBuffers(VkDevice device, VkCommandPool command_pool, uint32_t count) : command_buffers_(count)
{
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = count;

        //

        VkResult result = vkAllocateCommandBuffers(device, &allocate_info, command_buffers_.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAllocateCommandBuffers", result);
        }

        ASSERT(std::all_of(
                command_buffers_.cbegin(), command_buffers_.cend(),
                [](const VkCommandBuffer& command_buffer)
                {
                        return command_buffer != VK_NULL_HANDLE;
                }));

        device_ = device;
        command_pool_ = command_pool;
}

CommandBuffers::~CommandBuffers()
{
        destroy();
}

CommandBuffers::CommandBuffers(CommandBuffers&& from) noexcept
{
        move(&from);
}

CommandBuffers& CommandBuffers::operator=(CommandBuffers&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

const VkCommandBuffer& CommandBuffers::operator[](uint32_t index) const noexcept
{
        ASSERT(index < command_buffers_.size());

        return command_buffers_[index];
}

uint32_t CommandBuffers::count() const noexcept
{
        return command_buffers_.size();
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

void DescriptorSetLayout::move(DescriptorSetLayout* from) noexcept
{
        device_ = from->device_;
        descriptor_set_layout_ = from->descriptor_set_layout_;
        from->device_ = VK_NULL_HANDLE;
        from->descriptor_set_layout_ = VK_NULL_HANDLE;
}

DescriptorSetLayout::DescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& create_info)
{
        VkResult result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, &descriptor_set_layout_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDescriptorSetLayout", result);
        }

        ASSERT(descriptor_set_layout_ != VK_NULL_HANDLE);

        device_ = device;
}

DescriptorSetLayout::~DescriptorSetLayout()
{
        destroy();
}

DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& from) noexcept
{
        move(&from);
}

DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void DescriptorPool::move(DescriptorPool* from) noexcept
{
        device_ = from->device_;
        descriptor_pool_ = from->descriptor_pool_;
        from->device_ = VK_NULL_HANDLE;
        from->descriptor_pool_ = VK_NULL_HANDLE;
}

DescriptorPool::DescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo& create_info)
{
        VkResult result = vkCreateDescriptorPool(device, &create_info, nullptr, &descriptor_pool_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDescriptorPool", result);
        }

        ASSERT(descriptor_pool_ != VK_NULL_HANDLE);

        device_ = device;
}

DescriptorPool::~DescriptorPool()
{
        destroy();
}

DescriptorPool::DescriptorPool(DescriptorPool&& from) noexcept
{
        move(&from);
}

DescriptorPool& DescriptorPool::operator=(DescriptorPool&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void DescriptorSet::destroy() noexcept
{
        if (descriptor_set_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(descriptor_pool_ != VK_NULL_HANDLE);

                VkResult result = vkFreeDescriptorSets(device_, descriptor_pool_, 1, &descriptor_set_);
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkFreeDescriptorSets", result);
                }
        }
}

void DescriptorSet::move(DescriptorSet* from) noexcept
{
        device_ = from->device_;
        descriptor_pool_ = from->descriptor_pool_;
        descriptor_set_ = from->descriptor_set_;
        from->device_ = VK_NULL_HANDLE;
        from->descriptor_pool_ = VK_NULL_HANDLE;
        from->descriptor_set_ = VK_NULL_HANDLE;
}

DescriptorSet::DescriptorSet(
        VkDevice device,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout)
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(descriptor_pool != VK_NULL_HANDLE);
        ASSERT(descriptor_set_layout != VK_NULL_HANDLE);

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &descriptor_set_layout;

        VkResult result = vkAllocateDescriptorSets(device, &allocate_info, &descriptor_set_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAllocateDescriptorSets", result);
        }

        ASSERT(descriptor_set_ != VK_NULL_HANDLE);

        device_ = device;
        descriptor_pool_ = descriptor_pool;
}

DescriptorSet::~DescriptorSet()
{
        destroy();
}

DescriptorSet::DescriptorSet(DescriptorSet&& from) noexcept
{
        move(&from);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void DescriptorSets::destroy() noexcept
{
        if (!descriptor_sets_.empty())
        {
                ASSERT(device_ != VK_NULL_HANDLE);
                ASSERT(descriptor_pool_ != VK_NULL_HANDLE);

                VkResult result = vkFreeDescriptorSets(
                        device_, descriptor_pool_, descriptor_sets_.size(), descriptor_sets_.data());
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkFreeDescriptorSets", result);
                }
        }
}

void DescriptorSets::move(DescriptorSets* from) noexcept
{
        device_ = from->device_;
        descriptor_pool_ = from->descriptor_pool_;
        descriptor_sets_ = std::move(from->descriptor_sets_);
        from->device_ = VK_NULL_HANDLE;
        from->descriptor_pool_ = VK_NULL_HANDLE;
        from->descriptor_sets_ = std::vector<VkDescriptorSet>();
}

DescriptorSets::DescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptor_pool,
        const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts)
        : descriptor_sets_(descriptor_set_layouts.size())
{
        ASSERT(device != VK_NULL_HANDLE);
        ASSERT(descriptor_pool != VK_NULL_HANDLE);
        ASSERT(!descriptor_set_layouts.empty());
        ASSERT(std::all_of(
                descriptor_set_layouts.cbegin(), descriptor_set_layouts.cend(),
                [](const VkDescriptorSetLayout& descriptor_set_layout)
                {
                        return descriptor_set_layout != VK_NULL_HANDLE;
                }));

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = descriptor_pool;
        allocate_info.descriptorSetCount = descriptor_set_layouts.size();
        allocate_info.pSetLayouts = descriptor_set_layouts.data();

        VkResult result = vkAllocateDescriptorSets(device, &allocate_info, descriptor_sets_.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkAllocateDescriptorSets", result);
        }

        ASSERT(std::all_of(
                descriptor_sets_.cbegin(), descriptor_sets_.cend(),
                [](const VkDescriptorSet& descriptor_set)
                {
                        return descriptor_set != VK_NULL_HANDLE;
                }));

        device_ = device;
        descriptor_pool_ = descriptor_pool;
}

DescriptorSets::~DescriptorSets()
{
        destroy();
}

DescriptorSets::DescriptorSets(DescriptorSets&& from) noexcept
{
        move(&from);
}

DescriptorSets& DescriptorSets::operator=(DescriptorSets&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

const VkDescriptorSet& DescriptorSets::operator[](uint32_t index) const noexcept
{
        ASSERT(index < descriptor_sets_.size());

        return descriptor_sets_[index];
}

uint32_t DescriptorSets::count() const noexcept
{
        return descriptor_sets_.size();
}

//

void ImageHandle::destroy() noexcept
{
        if (image_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);

                vkDestroyImage(device_, image_, nullptr);
        }
}

void ImageHandle::move(ImageHandle* from) noexcept
{
        device_ = from->device_;
        image_ = from->image_;
        from->device_ = VK_NULL_HANDLE;
        from->image_ = VK_NULL_HANDLE;
}

ImageHandle::ImageHandle(VkDevice device, const VkImageCreateInfo& create_info)
{
        VkResult result = vkCreateImage(device, &create_info, nullptr, &image_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateImage", result);
        }

        ASSERT(image_ != VK_NULL_HANDLE);

        device_ = device;
}

ImageHandle::~ImageHandle()
{
        destroy();
}

ImageHandle::ImageHandle(ImageHandle&& from) noexcept
{
        move(&from);
}

ImageHandle& ImageHandle::operator=(ImageHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

//

void ImageViewHandle::destroy() noexcept
{
        if (image_view_ != VK_NULL_HANDLE)
        {
                ASSERT(device_ != VK_NULL_HANDLE);

                vkDestroyImageView(device_, image_view_, nullptr);
        }
}

void ImageViewHandle::move(ImageViewHandle* from) noexcept
{
        device_ = from->device_;
        image_view_ = from->image_view_;
        from->device_ = VK_NULL_HANDLE;
        from->image_view_ = VK_NULL_HANDLE;
}

ImageViewHandle::ImageViewHandle(VkDevice device, const VkImageViewCreateInfo& create_info)
{
        VkResult result = vkCreateImageView(device, &create_info, nullptr, &image_view_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateImageView", result);
        }

        ASSERT(image_view_ != VK_NULL_HANDLE);

        device_ = device;
}

ImageViewHandle::~ImageViewHandle()
{
        destroy();
}

ImageViewHandle::ImageViewHandle(ImageViewHandle&& from) noexcept
{
        move(&from);
}

ImageViewHandle& ImageViewHandle::operator=(ImageViewHandle&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
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

void Sampler::move(Sampler* from) noexcept
{
        device_ = from->device_;
        sampler_ = from->sampler_;
        from->device_ = VK_NULL_HANDLE;
        from->sampler_ = VK_NULL_HANDLE;
}

Sampler::Sampler(VkDevice device, const VkSamplerCreateInfo& create_info)
{
        VkResult result = vkCreateSampler(device, &create_info, nullptr, &sampler_);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateSampler", result);
        }

        ASSERT(sampler_ != VK_NULL_HANDLE);

        device_ = device;
}

Sampler::~Sampler()
{
        destroy();
}

Sampler::Sampler(Sampler&& from) noexcept
{
        move(&from);
}

Sampler& Sampler::operator=(Sampler&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}
}
