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

#include <string>
#include <vector>

namespace opengl
{
bool framebuffer_srgb();
bool current_buffer_is_srgb();

int framebuffer_samples();
long long max_texture_size();
long long max_texture_buffer_size();
long long max_variable_group_size_x();
long long max_variable_group_size_y();
long long max_variable_group_size_z();
long long max_variable_group_invocations();
long long max_fixed_group_size_x();
long long max_fixed_group_size_y();
long long max_fixed_group_size_z();
long long max_fixed_group_invocations();
long long max_work_group_count_x();
long long max_work_group_count_y();
long long max_work_group_count_z();
long long max_compute_shared_memory();
long long max_shader_storage_block_size();

void check_context(int major, int minor, const std::vector<std::string>& extensions);
void check_sizes(int sample_count, int depth_bits, int stencil_bits, int red_bits, int green_bits, int blue_bits, int alpha_bits);

const char* version();
const char* vendor();
const char* renderer();
std::vector<const char*> context_flags();
}
