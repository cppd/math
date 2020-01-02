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

#if defined(OPENGL_FOUND)

#include "shader_source.h"

constexpr const char fft_global_comp[]{
#include "dft_fft_global.comp.str"
};
constexpr const char fft_shared_comp[]{
#include "dft_fft_shared.comp.str"
};
constexpr const char bit_reverse_comp[]{
#include "dft_bit_reverse.comp.str"
};
constexpr const char copy_input_comp[]{
#include "dft_copy_input.comp.str"
};
constexpr const char copy_output_comp[]{
#include "dft_copy_output.comp.str"
};
constexpr const char mul_comp[]{
#include "dft_mul.comp.str"
};
constexpr const char mul_d_comp[]{
#include "dft_mul_d.comp.str"
};
constexpr const char show_vert[]{
#include "dft_show.vert.str"
};
constexpr const char show_frag[]{
#include "dft_show.frag.str"
};

namespace gpu_opengl
{
std::string dft_fft_global_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += fft_global_comp;
        return s;
}

std::string dft_fft_shared_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += fft_shared_comp;
        return s;
}

std::string dft_bit_reverse_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += bit_reverse_comp;
        return s;
}

std::string dft_copy_input_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += copy_input_comp;
        return s;
}

std::string dft_copy_output_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += copy_output_comp;
        return s;
}

std::string dft_mul_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += mul_comp;
        return s;
}

std::string dft_mul_d_comp(const std::string& insert)
{
        std::string s;
        s += insert;
        s += '\n';
        s += mul_d_comp;
        return s;
}

std::string dft_show_vert()
{
        return show_vert;
}

std::string dft_show_frag()
{
        return show_frag;
}
}

#endif
