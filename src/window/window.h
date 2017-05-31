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

#pragma once
#ifndef WINDOW_H
#define WINDOW_H

#include <SFML/Window/Context.hpp>
#include <SFML/Window/Window.hpp>
#include <memory>
#include <vector>

void create_gl_window_1x1(int major_gl_version, int minor_gl_version, const std::vector<std::string>& extensions,
                          int antialiasing_level, int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits,
                          int alpha_bits, sf::Window* wnd);
void create_gl_context_1x1(int major_gl_version, int minor_gl_version, const std::vector<std::string>& extensions,
                           std::unique_ptr<sf::Context>* context);

#endif
