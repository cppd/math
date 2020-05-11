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

#include "string_tree.h"

#include <src/com/error.h>

#include <stack>
#include <tuple>

constexpr size_t ROOT_NODE = 0;

StringTree::StringTree()
{
        m_nodes.emplace_back("");
}

size_t StringTree::add(const std::string& s)
{
        m_nodes.emplace_back(s);
        m_nodes[ROOT_NODE].children.push_back(m_nodes.size() - 1);
        return m_nodes.size() - 1;
}

size_t StringTree::add(size_t parent, const std::string& s)
{
        if (parent >= m_nodes.size())
        {
                error("Node parent out of range");
        }
        m_nodes.emplace_back(s);
        m_nodes[parent].children.push_back(m_nodes.size() - 1);
        return m_nodes.size() - 1;
}

std::string StringTree::text(unsigned indent) const
{
        std::string s;

        std::stack<std::tuple<size_t, unsigned>> stack({{ROOT_NODE, 0}});

        while (!stack.empty())
        {
                auto index = std::get<0>(stack.top());
                auto level = std::get<1>(stack.top());
                stack.pop();

                if (level > 0)
                {
                        if (!s.empty())
                        {
                                s += '\n';
                        }
                        s += std::string((level - 1) * indent, ' ');
                        s += m_nodes[index].name;
                }

                for (auto iter = m_nodes[index].children.crbegin(); iter != m_nodes[index].children.crend(); ++iter)
                {
                        stack.push({*iter, level + 1});
                }
        }

        return s;
}
