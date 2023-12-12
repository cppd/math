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

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ns
{
class StringTree final
{
        struct Node final
        {
                std::string name;
                std::vector<std::size_t> children;

                explicit Node(std::string s)
                        : name(std::move(s))
                {
                }
        };

        std::vector<Node> nodes_;

public:
        StringTree();

        std::size_t add(std::string s);
        std::size_t add(std::size_t parent, std::string s);

        [[nodiscard]] std::string text(unsigned indent) const;
};
}
