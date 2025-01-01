/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <cstdint>
#include <vector>

namespace ns::gpu::dft
{
std::vector<std::uint32_t> code_fft_global_comp();
std::vector<std::uint32_t> code_fft_shared_comp();
std::vector<std::uint32_t> code_bit_reverse_comp();
std::vector<std::uint32_t> code_copy_input_comp();
std::vector<std::uint32_t> code_copy_output_comp();
std::vector<std::uint32_t> code_mul_comp();
std::vector<std::uint32_t> code_mul_d_comp();
std::vector<std::uint32_t> code_view_vert();
std::vector<std::uint32_t> code_view_frag();
}
