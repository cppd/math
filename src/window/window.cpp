/*
Copyright (C) 2017 Topological Manifold

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

#include "gl/gl_query.h"
#include "gl_func/gl_functions.h"

#include <SFML/System/Err.hpp>

void create_gl_window_1x1(int major_gl_version, int minor_gl_version, const std::vector<std::string>& extensions,
                          int antialiasing_level, int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits,
                          int alpha_bits, sf::Window* wnd)
{
        sf::err().rdbuf(nullptr);

        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = antialiasing_level;
        cs.depthBits = depth_bits;
        cs.stencilBits = stencil_bits;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        wnd->create(sf::VideoMode(1, 1), "", sf::Style::None, cs);

        check_context(major_gl_version, minor_gl_version, extensions);
        check_bit_sizes(depth_bits, stencil_bits, antialiasing_level, red_bits, green_bits, blue_bits, alpha_bits);
        print_settings();
}

void create_gl_context_1x1(int major_gl_version, int minor_gl_version, const std::vector<std::string>& extensions,
                           std::unique_ptr<sf::Context>* context)
{
        sf::err().rdbuf(nullptr);

        sf::ContextSettings cs;
        cs.majorVersion = major_gl_version;
        cs.minorVersion = minor_gl_version;
        cs.antialiasingLevel = 0;
        cs.attributeFlags = sf::ContextSettings::Attribute::Core;

        *context = std::make_unique<sf::Context>(cs, 1, 1);

        check_context(major_gl_version, minor_gl_version, extensions);
        print_settings();
}
