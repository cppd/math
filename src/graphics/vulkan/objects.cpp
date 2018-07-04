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

#if defined(VULKAN_FOUND)

#include "objects.h"

#include "common.h"

#include "com/error.h"

namespace vulkan
{
void Instance::destroy() noexcept
{
        if (m_instance != VK_NULL_HANDLE)
        {
                vkDestroyInstance(m_instance, nullptr);
        }
}

void Instance::move(Instance* from) noexcept
{
        m_instance = from->m_instance;
        from->m_instance = VK_NULL_HANDLE;
}

Instance::Instance(const VkInstanceCreateInfo& create_info)
{
        VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateInstance", result);
        }

        ASSERT(m_instance != VK_NULL_HANDLE);
}

Instance::~Instance()
{
        destroy();
}

Instance::Instance(Instance&& from) noexcept
{
        move(&from);
}

Instance& Instance::operator=(Instance&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Instance::operator VkInstance() const
{
        return m_instance;
}

//

void DebugReportCallback::destroy() noexcept
{
        if (m_callback != VK_NULL_HANDLE)
        {
                ASSERT(m_instance != VK_NULL_HANDLE);

                vkDestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
        }
}

void DebugReportCallback::move(DebugReportCallback* from) noexcept
{
        m_instance = from->m_instance;
        m_callback = from->m_callback;
        from->m_instance = VK_NULL_HANDLE;
        from->m_callback = VK_NULL_HANDLE;
}

DebugReportCallback::DebugReportCallback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT& create_info)
{
        VkResult result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &m_callback);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDebugReportCallbackEXT", result);
        }

        m_instance = instance;

        ASSERT(m_callback != VK_NULL_HANDLE);
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

DebugReportCallback::operator VkDebugReportCallbackEXT() const
{
        return m_callback;
}

//

void Device::destroy() noexcept
{
        if (m_device != VK_NULL_HANDLE)
        {
                vkDestroyDevice(m_device, nullptr);
        }
}

void Device::move(Device* from) noexcept
{
        m_device = from->m_device;
        from->m_device = VK_NULL_HANDLE;
}

Device::Device() = default;

Device::Device(VkPhysicalDevice physical_device, const VkDeviceCreateInfo& create_info)
{
        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &m_device);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDevice", result);
        }

        ASSERT(m_device != VK_NULL_HANDLE);
}

Device::~Device()
{
        destroy();
}

Device::Device(Device&& from) noexcept
{
        move(&from);
}

Device& Device::operator=(Device&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Device::operator VkDevice() const
{
        return m_device;
}

//

void SurfaceKHR::destroy() noexcept
{
        if (m_surface != VK_NULL_HANDLE)
        {
                ASSERT(m_instance != VK_NULL_HANDLE);

                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
}

void SurfaceKHR::move(SurfaceKHR* from) noexcept
{
        m_instance = from->m_instance;
        m_surface = from->m_surface;
        from->m_instance = VK_NULL_HANDLE;
        from->m_surface = VK_NULL_HANDLE;
}

SurfaceKHR::SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for VkSurfaceKHR creation");
        }

        m_surface = create_surface(instance);

        ASSERT(m_surface != VK_NULL_HANDLE);

        m_instance = instance;
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

SurfaceKHR::operator VkSurfaceKHR() const
{
        return m_surface;
}

//

void SwapChainKHR::destroy() noexcept
{
        if (m_swap_chain != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroySwapchainKHR(m_device, m_swap_chain, nullptr);
        }
}

void SwapChainKHR::move(SwapChainKHR* from) noexcept
{
        m_device = from->m_device;
        m_swap_chain = from->m_swap_chain;
        from->m_device = VK_NULL_HANDLE;
        from->m_swap_chain = VK_NULL_HANDLE;
}

SwapChainKHR::SwapChainKHR() = default;

SwapChainKHR::SwapChainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info)
{
        VkResult result = vkCreateSwapchainKHR(device, &create_info, nullptr, &m_swap_chain);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateSwapchainKHR", result);
        }

        ASSERT(m_swap_chain != VK_NULL_HANDLE);

        m_device = device;
}

SwapChainKHR::~SwapChainKHR()
{
        destroy();
}

SwapChainKHR::SwapChainKHR(SwapChainKHR&& from) noexcept
{
        move(&from);
}

SwapChainKHR& SwapChainKHR::operator=(SwapChainKHR&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

SwapChainKHR::operator VkSwapchainKHR() const
{
        return m_swap_chain;
}

//

void ImageView::destroy() noexcept
{
        if (m_image_view != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyImageView(m_device, m_image_view, nullptr);
        }
}

void ImageView::move(ImageView* from) noexcept
{
        m_device = from->m_device;
        m_image_view = from->m_image_view;
        from->m_device = VK_NULL_HANDLE;
        from->m_image_view = VK_NULL_HANDLE;
}

ImageView::ImageView(VkDevice device, const VkImageViewCreateInfo& create_info)
{
        VkResult result = vkCreateImageView(device, &create_info, nullptr, &m_image_view);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateImageView", result);
        }

        ASSERT(m_image_view != VK_NULL_HANDLE);

        m_device = device;
}

ImageView::~ImageView()
{
        destroy();
}

ImageView::ImageView(ImageView&& from) noexcept
{
        move(&from);
}

ImageView& ImageView::operator=(ImageView&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

ImageView::operator VkImageView() const
{
        return m_image_view;
}

//

void ShaderModule::destroy() noexcept
{
        if (m_shader_module != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyShaderModule(m_device, m_shader_module, nullptr);
        }
}

void ShaderModule::move(ShaderModule* from) noexcept
{
        m_device = from->m_device;
        m_shader_module = from->m_shader_module;
        from->m_device = VK_NULL_HANDLE;
        from->m_shader_module = VK_NULL_HANDLE;
}

ShaderModule::ShaderModule(VkDevice device, const Span<const uint32_t>& code)
{
        static_assert(sizeof(uint32_t) == 4);

        if (code.empty())
        {
                error("Shader code size must be greater than 0");
        }

        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = 4 * code.size();
        create_info.pCode = code.data();

        //

        VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &m_shader_module);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateShaderModule", result);
        }

        ASSERT(m_shader_module != VK_NULL_HANDLE);

        m_device = device;
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

ShaderModule::operator VkShaderModule() const
{
        return m_shader_module;
}

//

Shader::Shader(VkDevice device, const Span<const uint32_t>& code, VkShaderStageFlagBits type)
        : m_module(device, code), m_stage(type)
{
}

const ShaderModule& Shader::module() const
{
        return m_module;
}

const VkShaderStageFlagBits& Shader::stage() const
{
        return m_stage;
}

//

VertexShader::VertexShader(VkDevice device, const Span<const uint32_t>& code) : Shader(device, code, VK_SHADER_STAGE_VERTEX_BIT)
{
}

TesselationControlShader::TesselationControlShader(VkDevice device, const Span<const uint32_t>& code)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
{
}

TesselationEvaluationShader ::TesselationEvaluationShader(VkDevice device, const Span<const uint32_t>& code)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
{
}

GeometryShader::GeometryShader(VkDevice device, const Span<const uint32_t>& code)
        : Shader(device, code, VK_SHADER_STAGE_GEOMETRY_BIT)
{
}

FragmentShader::FragmentShader(VkDevice device, const Span<const uint32_t>& code)
        : Shader(device, code, VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

ComputeShader::ComputeShader(VkDevice device, const Span<const uint32_t>& code)
        : Shader(device, code, VK_SHADER_STAGE_COMPUTE_BIT)
{
}

//

void RenderPass::destroy() noexcept
{
        if (m_render_pass != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyRenderPass(m_device, m_render_pass, nullptr);
        }
}

void RenderPass::move(RenderPass* from) noexcept
{
        m_device = from->m_device;
        m_render_pass = from->m_render_pass;
        from->m_device = VK_NULL_HANDLE;
        from->m_render_pass = VK_NULL_HANDLE;
}

RenderPass::RenderPass() = default;

RenderPass::RenderPass(VkDevice device, const VkRenderPassCreateInfo& create_info)
{
        VkResult result = vkCreateRenderPass(device, &create_info, nullptr, &m_render_pass);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateRenderPass", result);
        }

        ASSERT(m_render_pass != VK_NULL_HANDLE);

        m_device = device;
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

RenderPass::operator VkRenderPass() const
{
        return m_render_pass;
}

//

void PipelineLayout::destroy() noexcept
{
        if (m_pipeline_layout != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
        }
}

void PipelineLayout::move(PipelineLayout* from) noexcept
{
        m_device = from->m_device;
        m_pipeline_layout = from->m_pipeline_layout;
        from->m_device = VK_NULL_HANDLE;
        from->m_pipeline_layout = VK_NULL_HANDLE;
}

PipelineLayout::PipelineLayout() = default;

PipelineLayout::PipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo& create_info)
{
        VkResult result = vkCreatePipelineLayout(device, &create_info, nullptr, &m_pipeline_layout);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreatePipelineLayout", result);
        }

        ASSERT(m_pipeline_layout != VK_NULL_HANDLE);

        m_device = device;
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

PipelineLayout::operator VkPipelineLayout() const
{
        return m_pipeline_layout;
}

//

void Pipeline::destroy() noexcept
{
        if (m_pipeline != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
}

void Pipeline::move(Pipeline* from) noexcept
{
        m_device = from->m_device;
        m_pipeline = from->m_pipeline;
        from->m_device = VK_NULL_HANDLE;
        from->m_pipeline = VK_NULL_HANDLE;
}

Pipeline::Pipeline() = default;

Pipeline::Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info)
{
        VkPipelineCache pipeline_cache = VK_NULL_HANDLE;

        VkResult result = vkCreateGraphicsPipelines(device, pipeline_cache, 1, &create_info, nullptr, &m_pipeline);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateGraphicsPipelines", result);
        }

        ASSERT(m_pipeline != VK_NULL_HANDLE);

        m_device = device;
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

Pipeline::operator VkPipeline() const
{
        return m_pipeline;
}

//

void Framebuffer::destroy() noexcept
{
        if (m_framebuffer != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
        }
}

void Framebuffer::move(Framebuffer* from) noexcept
{
        m_device = from->m_device;
        m_framebuffer = from->m_framebuffer;
        from->m_device = VK_NULL_HANDLE;
        from->m_framebuffer = VK_NULL_HANDLE;
}

Framebuffer::Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info)
{
        VkResult result = vkCreateFramebuffer(device, &create_info, nullptr, &m_framebuffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateFramebuffer", result);
        }

        ASSERT(m_framebuffer != VK_NULL_HANDLE);

        m_device = device;
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

Framebuffer::operator VkFramebuffer() const
{
        return m_framebuffer;
}

//

void CommandPool::destroy() noexcept
{
        if (m_command_pool != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyCommandPool(m_device, m_command_pool, nullptr);
        }
}

void CommandPool::move(CommandPool* from) noexcept
{
        m_device = from->m_device;
        m_command_pool = from->m_command_pool;
        from->m_device = VK_NULL_HANDLE;
        from->m_command_pool = VK_NULL_HANDLE;
}

CommandPool::CommandPool() = default;

CommandPool::CommandPool(VkDevice device, const VkCommandPoolCreateInfo& create_info)
{
        VkResult result = vkCreateCommandPool(device, &create_info, nullptr, &m_command_pool);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateCommandPool", result);
        }

        ASSERT(m_command_pool != VK_NULL_HANDLE);

        m_device = device;
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

CommandPool::operator VkCommandPool() const
{
        return m_command_pool;
}

//

void Semaphore::destroy() noexcept
{
        if (m_semaphore != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroySemaphore(m_device, m_semaphore, nullptr);
        }
}

void Semaphore::move(Semaphore* from) noexcept
{
        m_device = from->m_device;
        m_semaphore = from->m_semaphore;
        from->m_device = VK_NULL_HANDLE;
        from->m_semaphore = VK_NULL_HANDLE;
}

Semaphore::Semaphore() = default;

Semaphore::Semaphore(VkDevice device)
{
        VkSemaphoreCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        //

        VkResult result = vkCreateSemaphore(device, &create_info, nullptr, &m_semaphore);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateSemaphore", result);
        }

        ASSERT(m_semaphore != VK_NULL_HANDLE);

        m_device = device;
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

Semaphore::operator VkSemaphore() const
{
        return m_semaphore;
}

//

void Fence::destroy() noexcept
{
        if (m_fence != VK_NULL_HANDLE)
        {
                ASSERT(m_device != VK_NULL_HANDLE);

                vkDestroyFence(m_device, m_fence, nullptr);
        }
}

void Fence::move(Fence* from) noexcept
{
        m_device = from->m_device;
        m_fence = from->m_fence;
        from->m_device = VK_NULL_HANDLE;
        from->m_fence = VK_NULL_HANDLE;
}

Fence::Fence() = default;

Fence::Fence(VkDevice device, bool signaled)
{
        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled)
        {
                fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        //

        VkResult result = vkCreateFence(device, &fence_info, nullptr, &m_fence);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkCreateFence", result);
        }

        ASSERT(m_fence != VK_NULL_HANDLE);

        m_device = device;
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

Fence::operator VkFence() const
{
        return m_fence;
}
}

#endif
