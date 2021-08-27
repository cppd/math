/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/numerical/vec.h>

#include <array>
#include <list>

namespace ns::geometry
{
template <std::size_t N>
class Ridge
{
        static_assert(N > 1);
        std::array<int, N - 1> vertices_;

public:
        explicit Ridge(std::array<int, N - 1>&& vertices) : vertices_(vertices)
        {
                ASSERT(std::is_sorted(vertices.cbegin(), vertices.cend()));
        }
        bool operator==(const Ridge& a) const
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (vertices_[i] != a.vertices_[i])
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
                        if (vertices_[i] < a.vertices_[i])
                        {
                                return true;
                        }
                        if (vertices_[i] > a.vertices_[i])
                        {
                                return false;
                        }
                }
                return false;
        }
        const std::array<int, N - 1>& vertices() const
        {
                return vertices_;
        }

        std::size_t hash() const
        {
                return compute_hash(vertices_);
        }
};

template <typename Facet>
class RidgeDataElement
{
        const Facet* facet_;
        int external_vertex_index_;

public:
        RidgeDataElement()
        {
                reset();
        }
        RidgeDataElement(const Facet* f, int p) : facet_(f), external_vertex_index_(p)
        {
        }
        int point() const
        {
                return facet_->vertices()[external_vertex_index_];
        }
        int vertex_index() const
        {
                return external_vertex_index_;
        }
        const Facet* facet() const
        {
                return facet_;
        }
        void reset()
        {
                facet_ = nullptr;
        }
};

template <int MAX_SIZE, typename Facet>
class RidgeDataC
{
        static_assert(MAX_SIZE > 1);

        std::array<RidgeDataElement<Facet>, MAX_SIZE> data_;
        int size_;

public:
        RidgeDataC(const Facet* facet, int external_point_index) : data_{{{facet, external_point_index}, {}}}, size_(1)
        {
        }

        void add(const Facet* facet, int external_point_index)
        {
                for (int i = 0; i < MAX_SIZE; ++i)
                {
                        if (data_[i].facet() == nullptr)
                        {
                                data_[i] = {facet, external_point_index};
                                ++size_;
                                return;
                        }
                }

                error("Add to ridge: too many facets exist in the link: facet " + to_string(facet->vertices())
                      + ", index " + to_string(external_point_index) + ", not ridge point "
                      + to_string(facet->vertices()[external_point_index]));
        }

        void remove(const Facet* facet)
        {
                for (int i = 0; i < MAX_SIZE; ++i)
                {
                        if (data_[i].facet() == facet)
                        {
                                data_[i].reset();
                                --size_;
                                return;
                        }
                }

                error("Remove ridge facet: facet not found in the link: facet " + to_string(facet->vertices()));
        }

        bool empty() const
        {
                return size_ == 0;
        }
        int size() const
        {
                return size_;
        }

        const RidgeDataElement<Facet>& operator[](unsigned i) const
        {
                ASSERT(i < MAX_SIZE);
                return data_[i];
        }
};

template <typename Facet>
using RidgeData2 = RidgeDataC<2, Facet>;

template <typename Facet>
class RidgeDataN
{
        std::list<RidgeDataElement<Facet>> data_;

public:
        RidgeDataN(const Facet* facet, int external_point_index)
        {
                data_.emplace_back(facet, external_point_index);
        }
        void add(const Facet* facet, int external_point_index)
        {
                data_.emplace_back(facet, external_point_index);
        }
        void remove(const Facet* facet)
        {
                for (auto iter = data_.cbegin(); iter != data_.cend(); ++iter)
                {
                        if (iter->facet() == facet)
                        {
                                data_.erase(iter);
                                return;
                        }
                }
                error("Remove ridge facet: facet not found in the link: facet " + to_string(facet->vertices()));
        }
        auto cbegin() const
        {
                return data_.cbegin();
        }
        auto cend() const
        {
                return data_.cend();
        }

        bool empty() const
        {
                return data_.empty();
        }
        int size() const
        {
                return data_.size();
        }
};

template <std::size_t N, typename Facet, template <typename...> typename Map, typename MapData>
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

template <std::size_t N, typename Facet, template <typename...> typename Map, typename MapData>
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

template <std::size_t N, typename Facet, template <typename...> typename Set>
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
}

namespace std
{
template <std::size_t N>
struct hash<::ns::geometry::Ridge<N>>
{
        std::size_t operator()(const ::ns::geometry::Ridge<N>& v) const
        {
                return v.hash();
        }
};
}
