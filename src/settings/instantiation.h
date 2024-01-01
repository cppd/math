/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/color/color.h>

#define TEMPLATE_INSTANTIATION_N(TEMPLATE_N) \
        TEMPLATE_N(3)                        \
        TEMPLATE_N(4)                        \
        TEMPLATE_N(5)                        \
        TEMPLATE_N(6)

#define TEMPLATE_INSTANTIATION_N_2(TEMPLATE_N) \
        TEMPLATE_N(2)                          \
        TEMPLATE_N(3)                          \
        TEMPLATE_N(4)                          \
        TEMPLATE_N(5)                          \
        TEMPLATE_N(6)

#define TEMPLATE_INSTANTIATION_N_2_A(TEMPLATE_N) \
        TEMPLATE_N(2)                            \
        TEMPLATE_N(3)                            \
        TEMPLATE_N(4)                            \
        TEMPLATE_N(5)                            \
        TEMPLATE_N(6)                            \
        TEMPLATE_N(7)

#define TEMPLATE_INSTANTIATION_N_M(TEMPLATE_N_M) \
        TEMPLATE_N_M(3, 1)                       \
        TEMPLATE_N_M(3, 2)                       \
        TEMPLATE_N_M(4, 1)                       \
        TEMPLATE_N_M(4, 2)                       \
        TEMPLATE_N_M(4, 3)                       \
        TEMPLATE_N_M(5, 1)                       \
        TEMPLATE_N_M(5, 2)                       \
        TEMPLATE_N_M(5, 3)                       \
        TEMPLATE_N_M(5, 4)                       \
        TEMPLATE_N_M(6, 1)                       \
        TEMPLATE_N_M(6, 2)                       \
        TEMPLATE_N_M(6, 3)                       \
        TEMPLATE_N_M(6, 4)                       \
        TEMPLATE_N_M(6, 5)

//

#define TEMPLATE_INSTANTIATION_N_T_IMPL_N(N, TEMPLATE_N_T) \
        TEMPLATE_N_T((N), float)                           \
        TEMPLATE_N_T((N), double)

#define TEMPLATE_INSTANTIATION_N_T(TEMPLATE_N_T)           \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(3, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(4, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(5, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(6, TEMPLATE_N_T)

#define TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE_N_T)         \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(2, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(3, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(4, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(5, TEMPLATE_N_T) \
        TEMPLATE_INSTANTIATION_N_T_IMPL_N(6, TEMPLATE_N_T)

//

#define TEMPLATE_INSTANTIATION_C(TEMPLATE_C) \
        TEMPLATE_C(::ns::color::Color)       \
        TEMPLATE_C(::ns::color::Spectrum)

//

#define TEMPLATE_INSTANTIATION_T_C_IMPL_T(T, TEMPLATE_T_C) \
        TEMPLATE_T_C(T, ::ns::color::Color)                \
        TEMPLATE_T_C(T, ::ns::color::Spectrum)

#define TEMPLATE_INSTANTIATION_T_C(TEMPLATE_T_C)               \
        TEMPLATE_INSTANTIATION_T_C_IMPL_T(float, TEMPLATE_T_C) \
        TEMPLATE_INSTANTIATION_T_C_IMPL_T(double, TEMPLATE_T_C)

//

#define TEMPLATE_INSTANTIATION_N_T_C_IMPL_N_T(N, T, TEMPLATE_N_T_C) \
        TEMPLATE_N_T_C((N), T, ::ns::color::Color)                  \
        TEMPLATE_N_T_C((N), T, ::ns::color::Spectrum)

#define TEMPLATE_INSTANTIATION_N_T_C_IMPL_N(N, TEMPLATE_N_T_C)            \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N_T((N), float, TEMPLATE_N_T_C) \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N_T((N), double, TEMPLATE_N_T_C)

#define TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE_N_T_C)           \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N(3, TEMPLATE_N_T_C) \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N(4, TEMPLATE_N_T_C) \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N(5, TEMPLATE_N_T_C) \
        TEMPLATE_INSTANTIATION_N_T_C_IMPL_N(6, TEMPLATE_N_T_C)
