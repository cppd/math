/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "models.h"

namespace ns::filter
{
#define INSTANTIATION(T)                                       \
        template Matrix<2, 2, T> continuous_white_noise(T, T); \
        template Matrix<3, 3, T> continuous_white_noise(T, T); \
        template Matrix<2, 2, T> discrete_white_noise(T, T);   \
        template Matrix<3, 3, T> discrete_white_noise(T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
