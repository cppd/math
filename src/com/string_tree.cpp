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

#include "string_tree.h"

#include <src/com/error.h>

#include <ranges>
#include <stack>
#include <string>
#include <tuple>

namespace ns
{
constexpr std::size_t ROOT_NODE = 0;

StringTree::StringTree()
{
        nodes_.emplace_back("");
}

std::size_t StringTree::add(std::string s)
{
        nodes_.emplace_back(std::move(s));
        nodes_[ROOT_NODE].children.push_back(nodes_.size() - 1);
        return nodes_.size() - 1;
}

std::size_t StringTree::add(const std::size_t parent, std::string s)
{
        if (parent >= nodes_.size())
        {
                error("Node parent out of range");
        }
        nodes_.emplace_back(std::move(s));
        nodes_[parent].children.push_back(nodes_.size() - 1);
        return nodes_.size() - 1;
}

std::string StringTree::text(const unsigned indent) const
{
        std::string res;

        std::stack<std::tuple<std::size_t, unsigned>> stack({
                {ROOT_NODE, 0}
        });

        while (!stack.empty())
        {
                const auto [index, level] = stack.top();
                stack.pop();

                if (level > 0)
                {
                        if (!res.empty())
                        {
                                res += '\n';
                        }
                        const unsigned level_indent = (level - 1) * indent;
                        res += std::string(level_indent, ' ');
                        res += nodes_[index].name;
                }

                for (const std::size_t child : std::ranges::reverse_view(nodes_[index].children))
                {
                        stack.emplace(child, level + 1);
                }
        }

        return res;
}
}
