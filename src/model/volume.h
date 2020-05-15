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

#include <src/color/color.h>
#include <src/numerical/matrix.h>

#include <array>
#include <vector>

namespace volume
{
template <size_t N>
struct Volume final
{
        struct Image
        {
                std::array<int, N> size;
                ColorFormat color_format;
                std::vector<std::byte> pixels;
        };

        Image image;
        Matrix<N + 1, N + 1, double> matrix;

        Volume() = default;
        Volume(const Volume&) = delete;
        Volume& operator=(const Volume&) = delete;
        Volume(Volume&&) = default;
        Volume& operator=(Volume&&) = default;
        ~Volume() = default;
};
}
