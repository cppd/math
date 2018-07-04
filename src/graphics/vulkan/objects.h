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

#include <functional>
#include <vulkan/vulkan.h>

namespace vulkan
{
class Instance
{
        VkInstance m_instance = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        Instance(const VkInstanceCreateInfo& create_info);
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&&) noexcept;
        Instance& operator=(Instance&&) noexcept;

        operator VkInstance() const;
};

class DebugReportCallback
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_callback = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(DebugReportCallback* from) noexcept;

public:
        DebugReportCallback(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT& create_info);
        ~DebugReportCallback();

        DebugReportCallback(const DebugReportCallback&) = delete;
        DebugReportCallback& operator=(const DebugReportCallback&) = delete;

        DebugReportCallback(DebugReportCallback&&) noexcept;
        DebugReportCallback& operator=(DebugReportCallback&&) noexcept;

        operator VkDebugReportCallbackEXT() const;
};

class Device
{
        VkDevice m_device = VK_NULL_HANDLE;

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

        operator VkDevice() const;
};

class SurfaceKHR
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SurfaceKHR* from) noexcept;

public:
        SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
        ~SurfaceKHR();

        SurfaceKHR(const SurfaceKHR&) = delete;
        SurfaceKHR& operator=(const SurfaceKHR&) = delete;

        SurfaceKHR(SurfaceKHR&&) noexcept;
        SurfaceKHR& operator=(SurfaceKHR&&) noexcept;

        operator VkSurfaceKHR() const;
};

class SwapChainKHR
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkSwapchainKHR m_swap_chain = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(SwapChainKHR* from) noexcept;

public:
        SwapChainKHR();
        SwapChainKHR(VkDevice device, const VkSwapchainCreateInfoKHR& create_info);
        ~SwapChainKHR();

        SwapChainKHR(const SwapChainKHR&) = delete;
        SwapChainKHR& operator=(const SwapChainKHR&) = delete;

        SwapChainKHR(SwapChainKHR&&) noexcept;
        SwapChainKHR& operator=(SwapChainKHR&&) noexcept;

        operator VkSwapchainKHR() const;
};

class ImageView
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkImageView m_image_view = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ImageView* from) noexcept;

public:
        ImageView(VkDevice device, const VkImageViewCreateInfo& create_info);
        ~ImageView();

        ImageView(const ImageView&) = delete;
        ImageView& operator=(const ImageView&) = delete;

        ImageView(ImageView&&) noexcept;
        ImageView& operator=(ImageView&&) noexcept;

        operator VkImageView() const;
};

class ShaderModule
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkShaderModule m_shader_module = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        ShaderModule(VkDevice device, const Span<const uint32_t>& code);
        ~ShaderModule();

        ShaderModule(const ShaderModule&) = delete;
        ShaderModule& operator=(const ShaderModule&) = delete;

        ShaderModule(ShaderModule&&) noexcept;
        ShaderModule& operator=(ShaderModule&&) noexcept;

        operator VkShaderModule() const;
};

class Shader
{
        ShaderModule m_module;
        VkShaderStageFlagBits m_stage;

protected:
        Shader(VkDevice device, const Span<const uint32_t>& code, VkShaderStageFlagBits stage);

public:
        const ShaderModule& module() const;
        const VkShaderStageFlagBits& stage() const;
};

class VertexShader final : public Shader
{
public:
        VertexShader(VkDevice device, const Span<const uint32_t>& code);
};

class TesselationControlShader final : public Shader
{
public:
        TesselationControlShader(VkDevice device, const Span<const uint32_t>& code);
};

class TesselationEvaluationShader final : public Shader
{
public:
        TesselationEvaluationShader(VkDevice device, const Span<const uint32_t>& code);
};

class GeometryShader final : public Shader
{
public:
        GeometryShader(VkDevice device, const Span<const uint32_t>& code);
};

class FragmentShader final : public Shader
{
public:
        FragmentShader(VkDevice device, const Span<const uint32_t>& code);
};

class ComputeShader final : public Shader
{
public:
        ComputeShader(VkDevice device, const Span<const uint32_t>& code);
};

class RenderPass
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

        operator VkRenderPass() const;
};

class PipelineLayout
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

        operator VkPipelineLayout() const;
};

class Pipeline
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Pipeline* from) noexcept;

public:
        Pipeline();
        Pipeline(VkDevice device, const VkGraphicsPipelineCreateInfo& create_info);
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(Pipeline&&) noexcept;
        Pipeline& operator=(Pipeline&&) noexcept;

        operator VkPipeline() const;
};

class Framebuffer
{
        VkDevice m_device = VK_NULL_HANDLE;
        VkFramebuffer m_framebuffer = VK_NULL_HANDLE;

        void destroy() noexcept;
        void move(Framebuffer* from) noexcept;

public:
        Framebuffer(VkDevice device, const VkFramebufferCreateInfo& create_info);
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(Framebuffer&&) noexcept;

        operator VkFramebuffer() const;
};

class CommandPool
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

        operator VkCommandPool() const;
};

class Semaphore
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

        operator VkSemaphore() const;
};

class Fence
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

        operator VkFence() const;
};
}

#endif
