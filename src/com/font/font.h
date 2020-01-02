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

#include <memory>
#include <optional>
#include <vector>

class Font final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        Font(int size_in_pixels);
        ~Font();

        void set_size(int size_in_pixels);

        struct Char
        {
                const unsigned char* image;
                int size, width, height, left, top, advance_x;
                char32_t code_point;
        };

        template <typename T>
        std::enable_if_t<std::is_same_v<T, char32_t>, std::optional<Char>> render(T code_point);
};
