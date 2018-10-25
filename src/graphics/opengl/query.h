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

#include <string>
#include <vector>

namespace opengl
{
bool framebuffer_srgb();
bool current_buffer_is_srgb();

int framebuffer_samples();
int max_texture_size();
int max_texture_buffer_size();
int max_work_group_size_x();
int max_work_group_size_y();
int max_work_group_size_z();
int max_work_group_invocations();
int max_work_group_count_x();
int max_work_group_count_y();
int max_work_group_count_z();
int max_compute_shared_memory();
int max_shader_storage_block_size();

std::string overview();

void check_context(int major, int minor, const std::vector<std::string>& extensions);
void check_bit_sizes(int depthBits, int stencilBits, int sample_count, int redBits, int greenBits, int blueBits, int alphaBits);
}
