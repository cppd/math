/*
Copyright (C) 2017-2020 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include <string>

namespace gpu_opengl
{
std::string dft_fft_global_comp(const std::string& insert);
std::string dft_fft_shared_comp(const std::string& insert);
std::string dft_bit_reverse_comp(const std::string& insert);
std::string dft_copy_input_comp(const std::string& insert);
std::string dft_copy_output_comp(const std::string& insert);
std::string dft_mul_comp(const std::string& insert);
std::string dft_mul_d_comp(const std::string& insert);
std::string dft_show_vert();
std::string dft_show_frag();
}

#endif
