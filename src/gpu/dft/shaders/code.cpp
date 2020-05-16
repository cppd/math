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

#include "code.h"

namespace gpu::dft
{
std::vector<uint32_t> code_fft_global_comp()
{
        return {
#include "dft_fft_global.comp.spr"
        };
}

std::vector<uint32_t> code_fft_shared_comp()
{
        return {
#include "dft_fft_shared.comp.spr"
        };
}

std::vector<uint32_t> code_bit_reverse_comp()
{
        return {
#include "dft_bit_reverse.comp.spr"
        };
}

std::vector<uint32_t> code_copy_input_comp()
{
        return {
#include "dft_copy_input.comp.spr"
        };
}

std::vector<uint32_t> code_copy_output_comp()
{
        return {
#include "dft_copy_output.comp.spr"
        };
}

std::vector<uint32_t> code_mul_comp()
{
        return {
#include "dft_mul.comp.spr"
        };
}

std::vector<uint32_t> code_mul_d_comp()
{
        return {
#include "dft_mul_d.comp.spr"
        };
}

std::vector<uint32_t> code_view_vert()
{
        return {
#include "dft_view.vert.spr"
        };
}

std::vector<uint32_t> code_view_frag()
{
        return {
#include "dft_view.frag.spr"
        };
}
}
