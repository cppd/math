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

#include "com/color/color.h"
#include "com/global_index.h"
#include "com/vec.h"

#include <string>
#include <vector>

template <size_t N>
class Image
{
        std::vector<Color> m_data;

        std::array<int, N> m_size;
        std::array<int, N> m_max;
        std::array<int, N> m_max_0;

        GlobalIndex<N, long long> m_global_index;
        std::array<long long, 1 << N> m_pixel_offsets;

        long long pixel_index(const std::array<int, N>& p) const;

        void read_from_srgba_pixels(const std::array<int, N>& size, const unsigned char* srgba_pixels);

        void resize(const std::array<int, N>& size);

        bool empty() const;

public:
        Image(const std::array<int, N>& size);

        Image(const std::array<int, N>& size, const std::vector<unsigned char>& srgba_pixels);

        template <size_t X = N>
        Image(std::enable_if_t<X == 2, const std::string&> file_name);

        template <typename T>
        Color texture(const Vector<N, T>& p) const;

        void set_pixel(const std::array<int, N>& p, const Color& color);

        template <size_t X = N>
        std::enable_if_t<X == 2> read_from_file(const std::string& file_name);

        template <size_t X = N>
        std::enable_if_t<X == 2> write_to_file(const std::string& file_name) const;

        template <size_t X = N>
        std::enable_if_t<X == 2> flip_vertically();
};
