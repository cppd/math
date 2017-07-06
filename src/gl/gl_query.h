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

#include <string>
#include <vector>

bool get_framebuffer_sRGB();
int get_framebuffer_samples();
int get_max_texture_size();
int get_max_texture_buffer_size();
int get_max_work_group_size_x();
int get_max_work_group_size_y();
int get_max_work_group_size_z();
int get_max_work_group_invocations();
int get_max_work_group_count_x();
int get_max_work_group_count_y();
int get_max_work_group_count_z();
int get_max_compute_shared_memory();
int get_max_shader_storage_block_size();

std::string graphics_overview();

void check_context(int major, int minor, const std::vector<std::string>& extensions);
void check_bit_sizes(int depthBits, int stencilBits, int antialiasing_level, int redBits, int greenBits, int blueBits,
                     int alphaBits);
