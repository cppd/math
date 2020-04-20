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
namespace
{
constexpr uint32_t fft_global_comp[]{
#include "dft_fft_global.comp.spr"
};
constexpr uint32_t fft_shared_comp[]{
#include "dft_fft_shared.comp.spr"
};
constexpr uint32_t bit_reverse_comp[]{
#include "dft_bit_reverse.comp.spr"
};
constexpr uint32_t copy_input_comp[]{
#include "dft_copy_input.comp.spr"
};
constexpr uint32_t copy_output_comp[]{
#include "dft_copy_output.comp.spr"
};
constexpr uint32_t mul_comp[]{
#include "dft_mul.comp.spr"
};
constexpr uint32_t mul_d_comp[]{
#include "dft_mul_d.comp.spr"
};
constexpr uint32_t view_vert[]{
#include "dft_view.vert.spr"
};
constexpr uint32_t view_frag[]{
#include "dft_view.frag.spr"
};
}

Span<const uint32_t> code_fft_global_comp()
{
        return fft_global_comp;
}

Span<const uint32_t> code_fft_shared_comp()
{
        return fft_shared_comp;
}

Span<const uint32_t> code_bit_reverse_comp()
{
        return bit_reverse_comp;
}

Span<const uint32_t> code_copy_input_comp()
{
        return copy_input_comp;
}

Span<const uint32_t> code_copy_output_comp()
{
        return copy_output_comp;
}

Span<const uint32_t> code_mul_comp()
{
        return mul_comp;
}

Span<const uint32_t> code_mul_d_comp()
{
        return mul_d_comp;
}

Span<const uint32_t> code_view_vert()
{
        return view_vert;
}

Span<const uint32_t> code_view_frag()
{
        return view_frag;
}
}
