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
#include <src/com/global_index.h>
#include <src/com/type/limit.h>
#include <src/numerical/vec.h>

#include <array>
#include <string>
#include <type_traits>
#include <vector>

template <size_t N>
class Image
{
        static std::array<int, N> initial_value();

        std::vector<Color> m_data;

        std::array<int, N> m_size = initial_value();
        std::array<int, N> m_max = initial_value();
        std::array<int, N> m_max_0 = initial_value();

        GlobalIndex<N, long long> m_global_index;
        std::array<long long, 1 << N> m_pixel_offsets;

        long long pixel_index(const std::array<int, N>& p) const;

        void load_from_pixels(
                const std::array<int, N>& size,
                ColorFormat color_format,
                const std::vector<std::byte>& pixels);

        void resize(const std::array<int, N>& size);

        bool empty() const;

public:
        explicit Image(const std::array<int, N>& size);

        Image(const std::array<int, N>& size, ColorFormat color_format, const std::vector<std::byte>& pixels);

        template <size_t X = N>
        explicit Image(std::enable_if_t<X == 2, const std::string&> file_name);

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
