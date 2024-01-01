/*
Copyright (C) 2017-2024 Topological Manifold

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

#if 0

#include "window.h"

#include "com/error.h"
#include "com/log.h"

// clang-format off
// include vulkan/vulkan.h before GLFW/glfw3.h
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

namespace ns::window
{
namespace
{
WindowEvent* g_event_interface = nullptr;

void callback_error(int /*error*/, const char* description)
{
        LOG(std::string("GLFW Error: ") + description);
}

void callback_framebuffer_size(GLFWwindow* /*window*/, int width, int height)
{
        g_event_interface->window_resized(width, height);
}

void callback_key(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
        if (action == GLFW_PRESS)
        {
                if (key == GLFW_KEY_F11)
                {
                        g_event_interface->window_keyboard_pressed(WindowEvent::KeyboardButton::F11);
                }
                if (key == GLFW_KEY_ESCAPE)
                {
                        g_event_interface->window_keyboard_pressed(WindowEvent::KeyboardButton::Escape);
                }
        }
}

void callback_cursor_pos(GLFWwindow* /*window*/, double x, double y)
{
        g_event_interface->window_mouse_moved(x, y);
}

void callback_scroll(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
        g_event_interface->window_mouse_wheel(yoffset);
}

void callback_mouse_button(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
        if (action == GLFW_PRESS)
        {
                if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                        g_event_interface->window_mouse_pressed(WindowEvent::MouseButton::Left);
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                        g_event_interface->window_mouse_pressed(WindowEvent::MouseButton::Right);
                }
        }
        else if (action == GLFW_RELEASE)
        {
                if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                        g_event_interface->window_mouse_released(WindowEvent::MouseButton::Left);
                }
                else if (button == GLFW_MOUSE_BUTTON_RIGHT)
                {
                        g_event_interface->window_mouse_released(WindowEvent::MouseButton::Right);
                }
        }
}

class WindowCounter final
{
        inline static std::atomic_int counter_ = 0;

public:
        WindowCounter()
        {
                if (++counter_ != 1)
                {
                        --counter_;
                        error("Too many GLFW windows");
                }
        }
        ~WindowCounter()
        {
                --counter_;
        }
        WindowCounter(const WindowCounter&) = delete;
        WindowCounter& operator=(const WindowCounter&) = delete;
        WindowCounter(WindowCounter&&) = delete;
        WindowCounter& operator=(WindowCounter&&) = delete;
};

class Impl final : public Window
{
        WindowCounter window_counter_;

        GLFWwindow* window_;

        WindowID system_handle() override
        {
#if defined(__linux__)
                return glfwGetX11Window(window_);
#elif defined(_WIN32)
                return glfwGetWin32Window(window_);
#else
#error This operating system is not supported
#endif
        }

        int width() const override
        {
                int width;
                int height;
                glfwGetFramebufferSize(window_, &width, &height);
                return width;
        }

        int height() const override
        {
                int width;
                int height;
                glfwGetFramebufferSize(window_, &width, &height);
                return height;
        }

        //VkSurfaceKHR create_surface(VkInstance instance) override
        //{
        //        VkSurfaceKHR surface = VK_NULL_HANDLE;
        //
        //        const VkResult result = glfwCreateWindowSurface(instance, window_, nullptr, &surface);
        //        if (result != VK_SUCCESS || surface == VK_NULL_HANDLE)
        //        {
        //                error("Failed to create Vulkan GLFW window surface");
        //        }
        //
        //        return surface;
        //}

        void pull_and_dispath_events(WindowEvent& window_event) override
        {
                //if (glfwWindowShouldClose(window))
                //{
                //        return;
                //}

                g_event_interface = &window_event;
                glfwPollEvents();
        }

public:
        Impl()
        {
                glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
                // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
                glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

                window_ = glfwCreateWindow(1, 1, "", nullptr, nullptr);

                if (!window_)
                {
                        error("Failed to create GLFW window");
                }

                glfwSetFramebufferSizeCallback(window_, callback_framebuffer_size);
                glfwSetKeyCallback(window_, callback_key);
                glfwSetCursorPosCallback(window_, callback_cursor_pos);
                glfwSetScrollCallback(window_, callback_scroll);
                glfwSetMouseButtonCallback(window_, callback_mouse_button);
        }

        ~Impl() override
        {
                glfwDestroyWindow(window_);
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

//

void window_init()
{
        glfwSetErrorCallback(callback_error);

        if (glfwInit() != GLFW_TRUE)
        {
                error("Failed to initialize GLFW");
        }
}

void window_terminate()
{
        glfwTerminate();
}

//

//std::vector<std::string> Window::instance_extensions()
//{
//        std::uint32_t count;
//        const char** extensions;

//        extensions = glfwGetRequiredInstanceExtensions(&count);

//        if (!extensions)
//        {
//                error("Failed to get GLFW required Vulkan instance extensions");
//        }
//        if (count < 1)
//        {
//                error("No GLFW required Vulkan instance extensions");
//        }

//        return std::vector<std::string>(extensions, extensions + count);
//}

std::unique_ptr<Window> create_window()
{
        return std::make_unique<Impl>();
}
}

#endif
