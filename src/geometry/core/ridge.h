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

#include "array_elements.h"

#include "com/error.h"
#include "com/hash.h"
#include "com/print.h"
#include "com/sort.h"
#include "com/vec.h"

#include <array>
#include <list>

template <size_t N>
class Ridge
{
        static_assert(N > 1);
        std::array<int, N - 1> m_vertices;

public:
        explicit Ridge(std::array<int, N - 1>&& vertices) : m_vertices(vertices)
        {
                ASSERT(std::is_sorted(vertices.cbegin(), vertices.cend()));
        }
        bool operator==(const Ridge& a) const
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (m_vertices[i] != a.m_vertices[i])
                        {
                                return false;
                        }
                }
                return true;
        }
        bool operator<(const Ridge& a) const
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (m_vertices[i] < a.m_vertices[i])
                        {
                                return true;
                        }
                        if (m_vertices[i] > a.m_vertices[i])
                        {
                                return false;
                        }
                }
                return false;
        }
        const std::array<int, N - 1>& vertices() const
        {
                return m_vertices;
        }

        size_t hash() const
        {
                return array_hash(m_vertices);
        }
};

namespace std
{
template <size_t N>
struct hash<Ridge<N>>
{
        size_t operator()(const Ridge<N>& v) const
        {
                return v.hash();
        }
};
}

template <typename Facet>
class RidgeDataElement
{
        const Facet* m_facet;
        int m_external_vertex_index;

public:
        RidgeDataElement()
        {
                reset();
        }
        RidgeDataElement(const Facet* f, int p) : m_facet(f), m_external_vertex_index(p)
        {
        }
        int point() const
        {
                return m_facet->vertices()[m_external_vertex_index];
        }
        int vertex_index() const
        {
                return m_external_vertex_index;
        }
        const Facet* facet() const
        {
                return m_facet;
        }
        void reset()
        {
                m_facet = nullptr;
        }
};

template <int MaxSize, typename Facet>
class RidgeDataC
{
        static_assert(MaxSize > 1);

        std::array<RidgeDataElement<Facet>, MaxSize> m_data;
        int m_size;

public:
        RidgeDataC(const Facet* facet, int external_point_index)
                : m_data{{{facet, external_point_index}, {}}}, m_size(1)
        {
        }

        void add(const Facet* facet, int external_point_index)
        {
                for (int i = 0; i < MaxSize; ++i)
                {
                        if (m_data[i].facet() == nullptr)
                        {
                                m_data[i] = {facet, external_point_index};
                                ++m_size;
                                return;
                        }
                }

                error("Add to ridge: too many facets exist in the link: facet " + to_string(facet->vertices()) +
                      ", index " + to_string(external_point_index) + ", not ridge point " +
                      to_string(facet->vertices()[external_point_index]));
        }

        void remove(const Facet* facet)
        {
                for (int i = 0; i < MaxSize; ++i)
                {
                        if (m_data[i].facet() == facet)
                        {
                                m_data[i].reset();
                                --m_size;
                                return;
                        }
                }

                error("Remove ridge facet: facet not found in the link: facet " + to_string(facet->vertices()));
        }

        bool empty() const
        {
                return m_size == 0;
        }
        int size() const
        {
                return m_size;
        }

        const RidgeDataElement<Facet>& operator[](unsigned i) const
        {
                ASSERT(i < MaxSize);
                return m_data[i];
        }
};

template <typename Facet>
using RidgeData2 = RidgeDataC<2, Facet>;

template <typename Facet>
class RidgeDataN
{
        std::list<RidgeDataElement<Facet>> m_data;

public:
        RidgeDataN(const Facet* facet, int external_point_index)
        {
                m_data.emplace_back(facet, external_point_index);
        }
        void add(const Facet* facet, int external_point_index)
        {
                m_data.emplace_back(facet, external_point_index);
        }
        void remove(const Facet* facet)
        {
                for (auto iter = m_data.cbegin(); iter != m_data.cend(); ++iter)
                {
                        if (iter->facet() == facet)
                        {
                                m_data.erase(iter);
                                return;
                        }
                }
                error("Remove ridge facet: facet not found in the link: facet " + to_string(facet->vertices()));
        }
        auto cbegin() const
        {
                return m_data.cbegin();
        }
        auto cend() const
        {
                return m_data.cend();
        }

        bool empty() const
        {
                return m_data.empty();
        }
        int size() const
        {
                return m_data.size();
        }
};

template <size_t N, typename Facet, template <typename...> typename Map, typename MapData>
void add_to_ridges(const Facet& facet, Map<Ridge<N>, MapData>* m)
{
        for (unsigned r = 0; r < N; ++r)
        {
                Ridge<N> ridge(sort(del_elem(facet.vertices(), r)));

                auto f = m->find(ridge);
                if (f == m->end())
                {
                        m->emplace(std::move(ridge), MapData(&facet, r));
                }
                else
                {
                        f->second.add(&facet, r);
                }
        }
}

template <size_t N, typename Facet, template <typename...> typename Map, typename MapData>
void remove_from_ridges(const Facet& facet, Map<Ridge<N>, MapData>* m)
{
        for (unsigned r = 0; r < N; ++r)
        {
                auto f = m->find(Ridge<N>(sort(del_elem(facet.vertices(), r))));

                ASSERT(f != m->end());

                f->second.remove(&facet);

                if (f->second.empty())
                {
                        m->erase(f);
                }
        }
}

template <size_t N, typename Facet, template <typename...> typename Set>
void add_to_ridges(const Facet& facet, int exclude_point, Set<Ridge<N>>* ridges)
{
        const std::array<int, N>& vertices = facet.vertices();
        for (unsigned r = 0; r < N; ++r)
        {
                if (vertices[r] != exclude_point)
                {
                        ridges->emplace(sort(del_elem(vertices, r)));
                }
        }
}
