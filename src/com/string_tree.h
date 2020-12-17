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

#include <string>
#include <vector>

class StringTree
{
        struct Node
        {
                std::string name;
                std::vector<size_t> children;
                explicit Node(std::string s) : name(std::move(s))
                {
                }
        };

        std::vector<Node> m_nodes;

public:
        StringTree();

        size_t add(std::string s);
        size_t add(size_t parent, std::string s);

        std::string text(unsigned indent) const;
};
