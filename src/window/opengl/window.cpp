/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "window.h"

#include "com/error.h"
#include "com/log.h"
#include "com/string/vector.h"
#include "graphics/opengl/overview.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/settings.h"

#include <SFML/System/Err.hpp>
#include <SFML/Window/Context.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

#if defined(_WIN32)
#include "graphics/opengl/functions/opengl_functions.h"
#include "window/manage.h"
#endif

constexpr int DEPTH_BITS = 24;
constexpr int STENCIL_BITS = 8;
constexpr int RED_BITS = 8;
constexpr int GREEN_BITS = 8;
constexpr int BLUE_BITS = 8;
constexpr int ALPHA_BITS = 8;

namespace opengl
{
namespace
{
#if defined(_WIN32)
void init_opengl_functions()
{
        // Для Винды адреса функций OpenGL зависят от контекста.
        // Теоретически, каждый контекст может иметь свои адреса,
        // поэтому не совсем правильно использовать общие адреса.
        opengl_functions::init();
}
#endif

void create_window_1x1(int major_gl_version, int minor_gl_version, const std::vector<std::string>& extensions,
                       int minimum_sample_count, int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits,
                       int alpha_bits, sf::Window* window)
{
        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = minimum_sample_count;
        cs.depthBits = depth_bits;
        cs.stencilBits = stencil_bits;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        window->create(sf::VideoMode(1, 1), "", sf::Style::None, cs);

#if defined(_WIN32)
        init_opengl_functions();
#endif

        opengl::check_context(major_gl_version, minor_gl_version, extensions);
        opengl::check_sizes(minimum_sample_count, depth_bits, stencil_bits, red_bits, green_bits, blue_bits, alpha_bits);

        LOG("\n-----OpenGL Window-----\n" + opengl::overview());
}

std::unique_ptr<sf::Context> create_context_1x1(int major_gl_version, int minor_gl_version,
                                                const std::vector<std::string>& extensions)
{
        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = 0;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        std::unique_ptr<sf::Context> context = std::make_unique<sf::Context>(cs, 1, 1);

#if defined(_WIN32)
        init_opengl_functions();
#endif

        opengl::check_context(major_gl_version, minor_gl_version, extensions);

        LOG("\n-----OpenGL Context-----\n" + opengl::overview());

        return context;
}

//

class Impl final : public Window
{
        sf::Window m_window;

        WindowID system_handle() const override
        {
                return m_window.getSystemHandle();
        }

        void set_vertical_sync_enabled(bool v) override
        {
                m_window.setVerticalSyncEnabled(v);
        }

        int width() const override
        {
                return m_window.getSize().x;
        }
        int height() const override
        {
                return m_window.getSize().y;
        }

        void display() override
        {
                m_window.display();
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        void pull_and_dispath_events(WindowEvent& window_event) override
        {
                sf::Event event;

                while (m_window.pollEvent(event))
                {
                        switch (event.type)
                        {
                        // case sf::Event::Closed:
                        //        break;
                        case sf::Event::KeyPressed:
                                switch (event.key.code)
                                {
                                case sf::Keyboard::F11:
                                        window_event.window_keyboard_pressed(WindowEvent::KeyboardButton::F11);
                                        break;
                                case sf::Keyboard::Escape:
                                        window_event.window_keyboard_pressed(WindowEvent::KeyboardButton::Escape);
                                        break;
                                }
                                break;
                        case sf::Event::MouseButtonPressed:
                                switch (event.mouseButton.button)
                                {
                                case sf::Mouse::Left:
                                        window_event.window_mouse_pressed(WindowEvent::MouseButton::Left);
                                        break;
                                case sf::Mouse::Right:
                                        window_event.window_mouse_pressed(WindowEvent::MouseButton::Right);
                                        break;
                                }
                                break;
                        case sf::Event::MouseButtonReleased:
                                switch (event.mouseButton.button)
                                {
                                case sf::Mouse::Left:
                                        window_event.window_mouse_released(WindowEvent::MouseButton::Left);
                                        break;
                                case sf::Mouse::Right:
                                        window_event.window_mouse_released(WindowEvent::MouseButton::Right);
                                        break;
                                }
                                break;
                        case sf::Event::MouseMoved:
                                window_event.window_mouse_moved(event.mouseMove.x, event.mouseMove.y);
                                break;
                        case sf::Event::MouseWheelScrolled:
                                window_event.window_mouse_wheel(event.mouseWheelScroll.delta);
                                break;
                        case sf::Event::Resized:
                                window_event.window_resized(event.size.width, event.size.height);
                                break;
                        }
                }
        }
#pragma GCC diagnostic pop

public:
        Impl(int minimum_sample_count)
        {
#if 0
                {
                        // Без этого создания контекста почему-то не сможет установиться
                        // sf::ContextSettings::antialiasingLevel в ненулевое значение
                        // далее при создании окна.
                        // В версии SFML 2.4.2 эта проблема исчезла.
                        Context opengl_context;
                }
#endif

                create_window_1x1(opengl::API_VERSION_MAJOR, opengl::API_VERSION_MINOR,
                                  string_vector(opengl::REQUIRED_EXTENSIONS), minimum_sample_count, DEPTH_BITS, STENCIL_BITS,
                                  RED_BITS, GREEN_BITS, BLUE_BITS, ALPHA_BITS, &m_window);
        }

        ~Impl() override
        {
#if defined(_WIN32)
                // Без этого вызова почему-то зависает деструктор окна SFML на Винде,
                // если это окно встроено в родительское окно.
                change_window_style_not_child(system_handle());
#endif
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
        sf::err().rdbuf(nullptr);
}

//

class Context::Impl
{
        std::unique_ptr<sf::Context> m_context;

public:
        Impl()
                : m_context(create_context_1x1(opengl::API_VERSION_MAJOR, opengl::API_VERSION_MINOR,
                                               string_vector(opengl::REQUIRED_EXTENSIONS)))
        {
        }
};

Context::Context()
{
        m_impl = std::make_unique<Context::Impl>();
}

Context::~Context() = default;

//

std::unique_ptr<Window> create_window(int minimum_sample_count)
{
        return std::make_unique<Impl>(minimum_sample_count);
}
}
