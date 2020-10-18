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

/*
Robert Sedgewick, Kevin Wayne.
Algorithms. Fourth edition.
Pearson Education, 2011.

1.5 Case Study: Union-Find
*/

#pragma once

#include <algorithm>
#include <numeric>
#include <type_traits>
#include <vector>

namespace geometry
{
// Weighted quick-union with path compression
template <typename T>
class UnionFind
{
        static_assert(std::is_integral_v<T>);

        std::vector<T> m_parent;
        std::vector<T> m_component_size;
        T m_component_count;

        T find_root(T p) const
        {
                while (p != m_parent[p])
                {
                        p = m_parent[p];
                }
                return p;
        }

        void compress_path(T p, T root)
        {
                while (m_parent[p] != root)
                {
                        T next = m_parent[p];
                        m_parent[p] = root;
                        p = next;
                }
        }

        T find_and_compress(T p)
        {
                T root = find_root(p);
                compress_path(p, root);
                return root;
        }

public:
        // std::type_identity_t для запрета template argument deduction
        explicit UnionFind(std::type_identity_t<T> count)
                : m_parent(count), m_component_size(count), m_component_count(count)
        {
                std::iota(m_parent.begin(), m_parent.end(), 0);
                std::fill(m_component_size.begin(), m_component_size.end(), 1);
        }

        bool add_connection(T p, T q)
        {
                T i = find_and_compress(p);
                T j = find_and_compress(q);

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

        //T count() const
        //{
        //        return m_component_count;
        //}

        //bool connected(T p, T q) const
        //{
        //        return find_root(p) == find_root(q);
        //}
};
}
