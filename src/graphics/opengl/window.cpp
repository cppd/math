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

#include "window.h"

#include "com/log.h"
#include "graphics/common_opengl.h"
#include "graphics/opengl/query.h"

#include <SFML/System/Err.hpp>

#if defined(_WIN32)
#include "graphics/opengl/functions/opengl_functions.h"
#endif

constexpr int ANTIALIASING_LEVEL = 4;
constexpr int DEPTH_BITS = 24;
constexpr int STENCIL_BITS = 8;
constexpr int RED_BITS = 8;
constexpr int GREEN_BITS = 8;
constexpr int BLUE_BITS = 8;
constexpr int ALPHA_BITS = 8;

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

std::unique_ptr<sf::Window> create_gl_window_1x1(int major_gl_version, int minor_gl_version,
                                                 const std::vector<std::string>& extensions, int antialiasing_level,
                                                 int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits,
                                                 int alpha_bits)
{
        sf::err().rdbuf(nullptr);

        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = antialiasing_level;
        cs.depthBits = depth_bits;
        cs.stencilBits = stencil_bits;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        std::unique_ptr<sf::Window> window = std::make_unique<sf::Window>(sf::VideoMode(1, 1), "", sf::Style::None, cs);

#if defined(_WIN32)
        init_opengl_functions();
#endif

        gpu::check_context(major_gl_version, minor_gl_version, extensions);
        gpu::check_bit_sizes(depth_bits, stencil_bits, antialiasing_level, red_bits, green_bits, blue_bits, alpha_bits);

        LOG("\n-----OpenGL Window-----\n" + gpu::graphics_overview());

        return window;
}

std::unique_ptr<sf::Context> create_gl_context_1x1(int major_gl_version, int minor_gl_version,
                                                   const std::vector<std::string>& extensions)
{
        sf::err().rdbuf(nullptr);

        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = 0;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        std::unique_ptr<sf::Context> context = std::make_unique<sf::Context>(cs, 1, 1);

#if defined(_WIN32)
        init_opengl_functions();
#endif

        gpu::check_context(major_gl_version, minor_gl_version, extensions);

        LOG("\n-----OpenGL Context-----\n" + gpu::graphics_overview());

        return context;
}
}

std::unique_ptr<sf::Window> create_gl_window_1x1()
{
        return create_gl_window_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions(), ANTIALIASING_LEVEL, DEPTH_BITS,
                                    STENCIL_BITS, RED_BITS, GREEN_BITS, BLUE_BITS, ALPHA_BITS);
}

std::unique_ptr<sf::Context> create_gl_context_1x1()
{
        return create_gl_context_1x1(MAJOR_GL_VERSION, MINOR_GL_VERSION, required_extensions());
}
