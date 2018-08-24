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

#if defined(VULKAN_FOUND) && defined(GLFW_FOUND)

#include "window.h"

#include "com/error.h"
#include "com/log.h"

// clang-format off
// Перед включением GLFW/glfw3.h надо включить vulkan/vulkan.h
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#else
#error This operating system is not supported
#endif
#include <GLFW/glfw3native.h>
// clang-format on

#include <atomic>

namespace
{
void glfw_error_callback(int /*error*/, const char* description)
{
        LOG(std::string("GLFW Error: ") + description);
}
}

void VulkanWindow::init()
{
        glfwSetErrorCallback(glfw_error_callback);

        if (glfwInit() != GLFW_TRUE)
        {
                error("Failed to initialize GLFW");
        }
}

void VulkanWindow::terminate() noexcept
{
        glfwTerminate();
}

std::vector<std::string> VulkanWindow::instance_extensions()
{
        uint32_t count;
        const char** extensions;

        extensions = glfwGetRequiredInstanceExtensions(&count);
        if (!extensions)
        {
                error("Failed to get GLFW required Vulkan instance extensions");
        }
        if (count < 1)
        {
                error("No GLFW required Vulkan instance extensions");
        }

        return std::vector<std::string>(extensions, extensions + count);
}

std::array<int, 2> VulkanWindow::screen_size()
{
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor)
        {
                error("Failed to find GLFW monitor");
        }

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode)
        {
                error("Failed to find GLFW monitor video mode");
        }

        return {mode->width, mode->height};
}

//

namespace
{
std::atomic_int global_glfw_window_count = 0;
// В один момент времени допускается существование только одного окна,
// поэтому можно использовать глобальную переменную
WindowEvent* global_event_interface = nullptr;

void callback_framebuffer_size(GLFWwindow* /*window*/, int width, int height)
{
        global_event_interface->window_resized(width, height);
}

void callback_key(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
        if (action == GLFW_PRESS)
        {
                if (key == GLFW_KEY_F11)
                {
                        global_event_interface->window_keyboard_pressed(WindowEvent::KeyboardButton::F11);
                }
                if (key == GLFW_KEY_ESCAPE)
                {
                        global_event_interface->window_keyboard_pressed(WindowEvent::KeyboardButton::Escape);
                }
        }
}

void callback_cursor_pos(GLFWwindow* /*window*/, double x, double y)
{
        global_event_interface->window_mouse_moved(x, y);
}

void callback_scroll(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
        global_event_interface->window_mouse_wheel(yoffset);
}

void callback_mouse_button(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
        if (action == GLFW_PRESS)
        {
                if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                        global_event_interface->window_mouse_pressed(WindowEvent::MouseButton::Left);
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                        global_event_interface->window_mouse_pressed(WindowEvent::MouseButton::Right);
                }
        }
        else if (action == GLFW_RELEASE)
        {
                if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                        global_event_interface->window_mouse_released(WindowEvent::MouseButton::Left);
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                        global_event_interface->window_mouse_released(WindowEvent::MouseButton::Right);
                }
        }
}

class VulkanWindowImplementation final : public VulkanWindow
{
        GLFWwindow* m_window;

        WindowID get_system_handle() override
        {
#if defined(__linux__)
                return glfwGetX11Window(m_window);
#elif defined(_WIN32)
                return glfwGetWin32Window(m_window);
#else
#error This operating system is not supported
#endif
        }

        int get_width() const override
        {
                int width, height;
                glfwGetFramebufferSize(m_window, &width, &height);
                return width;
        }

        int get_height() const override
        {
                int width, height;
                glfwGetFramebufferSize(m_window, &width, &height);
                return height;
        }

        VkSurfaceKHR create_surface(VkInstance instance) override
        {
                VkSurfaceKHR surface = VK_NULL_HANDLE;

                VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
                if (result != VK_SUCCESS || surface == VK_NULL_HANDLE)
                {
                        error("Failed to create Vulkan GLFW window surface");
                }

                return surface;
        }

        void pull_and_dispath_events() override
        {
                // if (glfwWindowShouldClose(window))
                // {
                //        return;
                //}

                glfwPollEvents();
        }

        void set_global_variables(WindowEvent* event_interface) const noexcept
        {
                ASSERT(event_interface);

                if (++global_glfw_window_count != 1)
                {
                        error_fatal("Too many GLFW windows");
                }
                global_event_interface = event_interface;
        }

        void clear_global_variables() const noexcept
        {
                global_event_interface = nullptr;
                --global_glfw_window_count;
        }

public:
        VulkanWindowImplementation(WindowEvent* event_interface)
        {
                set_global_variables(event_interface);

                try
                {
                        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                        // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
                        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

                        m_window = glfwCreateWindow(1, 1, "", nullptr, nullptr);

                        if (!m_window)
                        {
                                error("Failed to create GLFW window");
                        }

                        glfwSetFramebufferSizeCallback(m_window, callback_framebuffer_size);
                        glfwSetKeyCallback(m_window, callback_key);
                        glfwSetCursorPosCallback(m_window, callback_cursor_pos);
                        glfwSetScrollCallback(m_window, callback_scroll);
                        glfwSetMouseButtonCallback(m_window, callback_mouse_button);
                }
                catch (...)
                {
                        clear_global_variables();
                        throw;
                }
        }

        ~VulkanWindowImplementation() override
        {
                glfwDestroyWindow(m_window);
                clear_global_variables();
        }

        VulkanWindowImplementation(const VulkanWindowImplementation&) = delete;
        VulkanWindowImplementation& operator=(const VulkanWindowImplementation&) = delete;
        VulkanWindowImplementation(VulkanWindowImplementation&&) = delete;
        VulkanWindowImplementation& operator=(VulkanWindowImplementation&&) = delete;
};
}

std::unique_ptr<VulkanWindow> create_vulkan_window(WindowEvent* event_interface)
{
        return std::make_unique<VulkanWindowImplementation>(event_interface);
}

#endif
