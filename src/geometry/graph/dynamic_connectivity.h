/*
Copyright (C) 2017 Topological Manifold

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

/*
По книге

Robert Sedgewick, Kevin Wayne.
Algorithms. Fourth edition.
Pearson Education, 2011.

1.5 Case Study: Union-Find
*/

#pragma once

#include <algorithm>
#include <numeric>
#include <vector>

class WeightedQuickUnion
{
        std::vector<int> m_parent;
        std::vector<int> m_component_size;
        int m_component_count;

public:
        WeightedQuickUnion(int N) : m_parent(N), m_component_size(N), m_component_count(N)
        {
                std::iota(m_parent.begin(), m_parent.end(), 0);
                std::fill(m_component_size.begin(), m_component_size.end(), 1);
        }

        int find(int p) const
        {
                while (p != m_parent[p])
                {
                        p = m_parent[p];
                }
                return p;
        }

        bool add_connection(int p, int q)
        {
                int i = find(p);
                int j = find(q);

                if (i == j)
                {
                        return false;
                }

                // Меньшее дерево подсоединить к большему
                if (m_component_size[i] < m_component_size[j])
                {
                        m_parent[i] = j;
                        m_component_size[j] += m_component_size[i];
                }
                else
                {
                        m_parent[j] = i;
                        m_component_size[i] += m_component_size[j];
                }

                --m_component_count;

                return true;
        }

        int count() const
        {
                return m_component_count;
        }

        bool connected(int p, int q) const
        {
                return find(p) == find(q);
        }
};
