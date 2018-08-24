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

#include "show_vulkan.h"

#include "com/error.h"
#include "com/log.h"
#include "graphics/vulkan/window.h"
#include "show/vulkan/renderer/renderer.h"
#include "window/window_prop.h"

#include <atomic>
#include <thread>

namespace
{
class VulkanObject final : public IShowVulkan, public WindowEvent
{
        WindowID m_win_parent;
        std::thread m_thread;
        std::atomic_bool m_stop{false};

        void parent_resized() override
        {
                LOG("parent resized");
        }
        void mouse_wheel(double) override
        {
                LOG("mouse wheel");
        }

        void window_keyboard_pressed(KeyboardButton /*button*/) override
        {
                LOG("window keyboard pressed");
        }
        void window_mouse_pressed(MouseButton /*button*/) override
        {
                LOG("window mouse pressed");
        }
        void window_mouse_released(MouseButton /*button*/) override
        {
                LOG("window mouse released");
        }
        void window_mouse_moved(int /*x*/, int /*y*/) override
        {
                LOG("window mouse moved");
        }
        void window_mouse_wheel(int /*delta*/) override
        {
                LOG("window mouse wheel");
        }
        void window_resized(int /*width*/, int /*height*/) override
        {
                LOG("window resized");
        }

        void loop();
        void loop_thread();

public:
        VulkanObject(WindowID win_parent) : m_win_parent(win_parent)
        {
                m_thread = std::thread(&VulkanObject::loop_thread, this);
        }

        ~VulkanObject() override
        {
                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }
};

void VulkanObject::loop()
{
        std::unique_ptr<VulkanWindow> window = create_vulkan_window(this);
        move_window_to_parent(window->get_system_handle(), m_win_parent);

        std::unique_ptr<VulkanRenderer> renderer = create_vulkan_renderer(
                VulkanWindow::instance_extensions(), [&window](VkInstance instance) { return window->create_surface(instance); });

        while (true)
        {
                if (m_stop)
                {
                        return;
                }

                window->pull_and_dispath_events();

                renderer->draw();
        }
}

void VulkanObject::loop_thread()
{
        try
        {
                loop();
        }
        catch (std::exception& e)
        {
                error_fatal(e.what());
        }
        catch (...)
        {
                error_fatal("Unknown Error. Thread ended.");
        }
}
}

std::unique_ptr<IShowVulkan> create_show_vulkan(WindowID win_parent)
{
        return std::make_unique<VulkanObject>(win_parent);
}

#endif
