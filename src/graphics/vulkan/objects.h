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

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
class Instance
{
        VkInstance m_instance = VK_NULL_HANDLE;

        void create(int api_version_major, int api_version_minor, std::vector<const char*> required_extensions,
                    const std::vector<const char*>& required_validation_layers);
        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        Instance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_extensions,
                 const std::vector<const char*>& required_validation_layers);
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

        void create(VkInstance instance);
        void destroy() noexcept;
        void move(DebugReportCallback* from) noexcept;

public:
        DebugReportCallback(VkInstance instance);
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

        void create(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
                    const std::vector<const char*>& required_extensions,
                    const std::vector<const char*>& required_validation_layers);
        void destroy() noexcept;
        void move(Device* from) noexcept;

public:
        Device();
        Device(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
               const std::vector<const char*>& required_extensions, const std::vector<const char*>& required_validation_layers);
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

        void create(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
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

        void create(VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode,
                    VkExtent2D extent, uint32_t image_count, VkSurfaceTransformFlagBitsKHR transform,
                    unsigned graphics_family_index, unsigned presentation_family_index);
        void destroy() noexcept;
        void move(SwapChainKHR* from) noexcept;

public:
        SwapChainKHR();
        SwapChainKHR(VkDevice device, VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode,
                     VkExtent2D extent, uint32_t image_count, VkSurfaceTransformFlagBitsKHR transform,
                     unsigned graphics_family_index, unsigned presentation_family_index);
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

        void create(VkDevice device, VkImage image, VkFormat format);
        void destroy() noexcept;
        void move(ImageView* from) noexcept;

public:
        ImageView(VkDevice device, VkImage image, VkFormat format);
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

        void create(VkDevice device, const std::vector<uint8_t>& code);
        void destroy() noexcept;
        void move(ShaderModule* from) noexcept;

public:
        ShaderModule(VkDevice device, const std::vector<uint8_t>& code);
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
        Shader(VkDevice device, const std::vector<uint8_t>& code, VkShaderStageFlagBits stage);

public:
        const ShaderModule& module() const;
        const VkShaderStageFlagBits& stage() const;
};

class VertexShader final : public Shader
{
public:
        VertexShader(VkDevice device, const std::vector<uint8_t>& code);
};

class TesselationControlShader final : public Shader
{
public:
        TesselationControlShader(VkDevice device, const std::vector<uint8_t>& code);
};

class TesselationEvaluationShader final : public Shader
{
public:
        TesselationEvaluationShader(VkDevice device, const std::vector<uint8_t>& code);
};

class GeometryShader final : public Shader
{
public:
        GeometryShader(VkDevice device, const std::vector<uint8_t>& code);
};

class FragmentShader final : public Shader
{
public:
        FragmentShader(VkDevice device, const std::vector<uint8_t>& code);
};

class ComputeShader final : public Shader
{
public:
        ComputeShader(VkDevice device, const std::vector<uint8_t>& code);
};

class VulkanInstance
{
        Instance m_instance;
        std::optional<DebugReportCallback> m_callback;

        SurfaceKHR m_surface;

        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;

        Device m_device;

        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_compute_queue = VK_NULL_HANDLE;
        VkQueue m_presentation_queue = VK_NULL_HANDLE;

        SwapChainKHR m_swap_chain;
        std::vector<VkImage> m_swap_chain_images;
        VkFormat m_swap_chain_image_format;
        VkExtent2D m_swap_chain_extent;

        std::vector<ImageView> m_image_views;

public:
        VulkanInstance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_instance_extensions,
                       const std::vector<const char*>& required_device_extensions,
                       const std::vector<const char*>& required_validation_layers,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface,
                       const std::vector<uint8_t>& vertex_shader_code, const std::vector<uint8_t>& fragment_shader_code);

        VkInstance instance() const;
};
}

#endif
