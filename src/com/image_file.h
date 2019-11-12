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

#pragma once

#include "span.h"

#include <string>
#include <vector>

void save_grayscale_image_to_file(const std::string& file_name, int width, int height, Span<const std::uint_least8_t> pixels);
void save_srgb_image_to_file(const std::string& file_name, int width, int height, const std::vector<std::uint_least8_t>& pixels);

void load_srgba_image_from_file(const std::string& file_name, int* width, int* height, std::vector<std::uint_least8_t>* pixels);
void flip_srgba_image_vertically(int width, int height, std::vector<std::uint_least8_t>* pixels);
