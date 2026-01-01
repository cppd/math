/*
Copyright (C) 2017-2026 Topological Manifold

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

namespace ns
{
// Weighted quick-union with path compression
template <typename T>
class UnionFind final
{
        static_assert(std::is_integral_v<T>);

        std::vector<T> parent_;
        std::vector<T> component_size_;
        T component_count_;

        [[nodiscard]] T find_root(T p) const
        {
                while (p != parent_[p])
                {
                        p = parent_[p];
                }
                return p;
        }

        void compress_path(T p, const T root)
        {
                while (parent_[p] != root)
                {
                        const T next = parent_[p];
                        parent_[p] = root;
                        p = next;
                }
        }

        [[nodiscard]] T find_and_compress(const T p)
        {
                const T root = find_root(p);
                compress_path(p, root);
                return root;
        }

public:
        explicit UnionFind(std::type_identity_t<T> count)
                : parent_(count),
                  component_size_(count),
                  component_count_(count)
        {
                std::iota(parent_.begin(), parent_.end(), 0);
                std::fill(component_size_.begin(), component_size_.end(), 1);
        }

        bool add_connection(const T p, const T q)
        {
                const T i = find_and_compress(p);
                const T j = find_and_compress(q);

                if (i == j)
                {
                        return false;
                }

                if (component_size_[i] < component_size_[j])
                {
                        parent_[i] = j;
                        component_size_[j] += component_size_[i];
                }
                else
                {
                        parent_[j] = i;
                        component_size_[i] += component_size_[j];
                }

                --component_count_;

                return true;
        }

        // T count() const
        // {
        //         return component_count_;
        // }

        // bool connected(const T p, const T q) const
        // {
        //         return find_root(p) == find_root(q);
        // }
};
}
