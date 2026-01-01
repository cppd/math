/*
Copyright (C) 2017-2026 Topological Manifold

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

#define FILTER_TEMPLATE_INSTANTIATION_N_T_IMPL_N(N, TEMPLATE_N_T) \
        TEMPLATE_N_T((N), float)                                  \
        TEMPLATE_N_T((N), double)                                 \
        TEMPLATE_N_T((N), long double)

#define FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE_N_T)           \
        FILTER_TEMPLATE_INSTANTIATION_N_T_IMPL_N(1, TEMPLATE_N_T) \
        FILTER_TEMPLATE_INSTANTIATION_N_T_IMPL_N(2, TEMPLATE_N_T) \
        FILTER_TEMPLATE_INSTANTIATION_N_T_IMPL_N(3, TEMPLATE_N_T) \
        FILTER_TEMPLATE_INSTANTIATION_N_T_IMPL_N(4, TEMPLATE_N_T)

#define FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE_T) \
        TEMPLATE_T(float)                           \
        TEMPLATE_T(double)                          \
        TEMPLATE_T(long double)
