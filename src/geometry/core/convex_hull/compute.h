/*
Copyright (C) 2017-2024 Topological Manifold

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
Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

Mark de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars.
Computational Geometry. Algorithms and Applications. Third Edition.
Springer-Verlag Berlin Heidelberg, 2008.

Handbook of Discrete and Computational Geometry.
Edited by Jacob E. Goodman, Joseph O’Rourke.
Chapman & Hall/CRC, 2004.
*/

/*
Convex hull
Randomized incremental algorithm.
(Computational Geometry. Algorithms and Applications. 11 Convex Hulls)

Delaunay objects
The projection to the n-space of the lower convex hull of the points
(x(0), ..., x(n), x(0)^2 + ... + x(n)^2).
(Discrete and computational geometry. 4.4 CONVEX HULL REVISITED)
*/

#pragma once

#include "facet.h"
#include "facet_connector.h"
#include "facet_storage.h"
#include "simplex_points.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/thread_pool.h>
#include <src/com/type/concept.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <algorithm>
#include <array>
#include <barrier>
#include <cstddef>
#include <vector>

namespace ns::geometry::core::convex_hull
{
namespace compute_implementation
{
template <typename S, typename C>
int thread_count_for_horizon()
{
        static_assert(Integral<S> && Integral<C>);

        const int hc = hardware_concurrency();
        return std::max(hc - 1, 1);
}

template <std::size_t N, typename S, typename C>
void create_initial_convex_hull(
        const std::vector<numerical::Vector<N, S>>& points,
        std::array<int, N + 1>* const vertices,
        FacetList<Facet<N, S, C>>* const facets)
{
        *vertices = find_simplex_points<N, S, C>(points);

        facets->clear();
        for (std::size_t i = 0; i < N + 1; ++i)
        {
                facets->emplace_back(points, del_elem(*vertices, i), (*vertices)[i]);
                std::prev(facets->end())->set_iter(std::prev(facets->cend()));
        }

        constexpr int RIDGE_COUNT = BINOMIAL<N + 1, N - 1>;

        FacetConnector<N, Facet<N, S, C>> facet_connector(RIDGE_COUNT);
        for (Facet<N, S, C>& facet : *facets)
        {
                facet_connector.connect(&facet, -1);
        }
}

template <typename Point, typename Facet>
void create_initial_conflict_lists(
        const std::vector<Point>& points,
        const std::vector<unsigned char>& enabled,
        FacetList<Facet>* const facets,
        std::vector<FacetStorage<Facet>>* const point_conflicts)
{
        for (Facet& facet : *facets)
        {
                for (std::size_t i = 0; i < points.size(); ++i)
                {
                        if (enabled[i] && facet.visible_from_point(points, i))
                        {
                                (*point_conflicts)[i].insert(&facet);
                                facet.add_conflict_point(i);
                        }
                }
        }
}

template <typename Point, typename Facet>
void add_conflict_points_to_new_facet(
        const std::vector<Point>& points,
        const int point,
        std::vector<signed char>* const unique_points,
        const Facet* const facet_0,
        const Facet* const facet_1,
        Facet* const new_facet)
{
        for (const int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 1;

                if (p != point && new_facet->visible_from_point(points, p))
                {
                        new_facet->add_conflict_point(p);
                }
        }

        for (const int p : facet_1->conflict_points())
        {
                if ((*unique_points)[p] != 0)
                {
                        continue;
                }

                if (p != point && new_facet->visible_from_point(points, p))
                {
                        new_facet->add_conflict_point(p);
                }
        }

        for (const int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 0;
        }
}

template <typename Facet>
void erase_visible_facets_from_conflict_points(
        const unsigned thread_id,
        const unsigned thread_count,
        std::vector<FacetStorage<Facet>>* const point_conflicts,
        const int point)
{
        for (const Facet* const facet : (*point_conflicts)[point])
        {
                for (const int p : facet->conflict_points())
                {
                        if (p != point && (p % thread_count) == thread_id)
                        {
                                (*point_conflicts)[p].erase(facet);
                        }
                }
        }
}

template <typename Facet>
void add_new_facets_to_conflict_points(
        const unsigned thread_id,
        const unsigned thread_count,
        const std::vector<FacetList<Facet>>& new_facets_vector,
        std::vector<FacetStorage<Facet>>* const point_conflicts)
{
        for (const FacetList<Facet>& facet_list : new_facets_vector)
        {
                for (const Facet& facet : facet_list)
                {
                        for (const int p : facet.conflict_points())
                        {
                                if ((p % thread_count) == thread_id)
                                {
                                        (*point_conflicts)[p].insert(&facet);
                                }
                        }
                }
        }
}

template <typename Point, typename Facet>
void create_facets_for_point_and_horizon(
        const unsigned thread_id,
        const unsigned thread_count,
        const std::vector<Point>& points,
        const int point,
        const std::vector<FacetStorage<Facet>>& point_conflicts,
        std::vector<std::vector<signed char>>* const unique_points_work,
        std::vector<FacetList<Facet>>* const new_facets_vector)
{
        ASSERT(new_facets_vector->size() == thread_count);
        ASSERT(unique_points_work->size() == thread_count);

        std::vector<signed char>* const unique_points = &(*unique_points_work)[thread_id];
        FacetList<Facet>* const new_facets = &(*new_facets_vector)[thread_id];

        new_facets->clear();

        std::size_t ridge_count = 0;
        std::size_t index = thread_id;
        for (const Facet* const facet : point_conflicts[point])
        {
                for (unsigned r = 0; r < facet->vertices().size(); ++r)
                {
                        Facet* const link_facet = facet->link(r);

                        if (link_facet->marked_as_visible())
                        {
                                continue;
                        }

                        if (ridge_count != index)
                        {
                                ++ridge_count;
                                continue;
                        }

                        ++ridge_count;
                        index += thread_count;

                        const int link_index = link_facet->find_link_index(facet);

                        new_facets->emplace_back(
                                points, set_elem(facet->vertices(), r, point), link_facet->vertices()[link_index],
                                *link_facet);

                        Facet* const new_facet = &(*std::prev(new_facets->end()));
                        new_facet->set_iter(std::prev(new_facets->cend()));

                        new_facet->set_link(new_facet->find_index_for_point(point), link_facet);
                        link_facet->set_link(link_index, new_facet);

                        add_conflict_points_to_new_facet(points, point, unique_points, facet, link_facet, new_facet);
                }
        }
}

template <std::size_t N, typename S, typename C>
void create_horizon_facets(
        const unsigned thread_id,
        const unsigned thread_count,
        const std::vector<numerical::Vector<N, S>>& points,
        const int point,
        std::vector<FacetStorage<Facet<N, S, C>>>* const point_conflicts,
        std::vector<std::vector<signed char>>* const unique_points_work,
        std::vector<FacetList<Facet<N, S, C>>>* const new_facets_vector,
        std::barrier<>* const barrier)
{
        try
        {
                create_facets_for_point_and_horizon(
                        thread_id, thread_count, points, point, *point_conflicts, unique_points_work,
                        new_facets_vector);
        }
        catch (...)
        {
                barrier->arrive_and_wait();
                throw;
        }
        barrier->arrive_and_wait();

        // erase first, then add.
        // this reduces the amount of searching.
        erase_visible_facets_from_conflict_points(thread_id, thread_count, point_conflicts, point);
        add_new_facets_to_conflict_points(thread_id, thread_count, *new_facets_vector, point_conflicts);
}

template <typename T>
std::size_t calculate_facet_count(const std::vector<T>& facets)
{
        std::size_t res = 0;
        for (const T& facet : facets)
        {
                res += facet.size();
        }
        return res;
}

template <std::size_t N, typename S, typename C>
void add_point_to_convex_hull(
        const std::vector<numerical::Vector<N, S>>& points,
        const int point,
        FacetList<Facet<N, S, C>>* const facets,
        std::vector<FacetStorage<Facet<N, S, C>>>* const point_conflicts,
        ThreadPool* const thread_pool,
        std::barrier<>* const barrier,
        std::vector<std::vector<signed char>>* const unique_points_work)
{
        if ((*point_conflicts)[point].size() == 0)
        {
                // the point is inside the convex hull
                return;
        }

        if ((*point_conflicts)[point].size() >= facets->size())
        {
                error("All facets are visible from the point");
        }

        for (const Facet<N, S, C>* const facet : (*point_conflicts)[point])
        {
                facet->mark_as_visible();
        }

        std::vector<FacetList<Facet<N, S, C>>> new_facets(thread_pool->thread_count());

        if (thread_pool->thread_count() > 1)
        {
                thread_pool->run(
                        [&](const unsigned thread_id, const unsigned thread_count)
                        {
                                create_horizon_facets(
                                        thread_id, thread_count, points, point, point_conflicts, unique_points_work,
                                        &new_facets, barrier);
                        });
        }
        else
        {
                constexpr unsigned THREAD_ID = 0;
                constexpr unsigned THREAD_COUNT = 1;

                create_horizon_facets(
                        THREAD_ID, THREAD_COUNT, points, point, point_conflicts, unique_points_work, &new_facets,
                        barrier);
        }

        // Erase visible facets
        for (const Facet<N, S, C>* const facet : (*point_conflicts)[point])
        {
                facets->erase(facet->iter());
        }

        (*point_conflicts)[point].clear();

        {
                // Connect facets, excluding horizon facets
                const std::size_t facet_count = calculate_facet_count(new_facets);
                const std::size_t ridge_count = (N - 1) * facet_count / 2;

                FacetConnector<N, Facet<N, S, C>> facet_connector(ridge_count);
                for (FacetList<Facet<N, S, C>>& facet_list : new_facets)
                {
                        for (Facet<N, S, C>& facet : facet_list)
                        {
                                facet_connector.connect(&facet, point);
                        }
                }
        }

        for (std::size_t i = 0; i < new_facets.size(); ++i)
        {
                facets->splice(facets->cend(), new_facets[i]);
        }
}

template <typename C, std::size_t N, typename S>
FacetList<Facet<N, S, C>> compute_convex_hull(
        const std::vector<numerical::Vector<N, S>>& points,
        progress::Ratio* const progress)
{
        static_assert(N > 1);

        if (points.size() < N + 1)
        {
                error("Error point count " + to_string(points.size()) + " for convex hull in " + space_name(N));
        }

        FacetList<Facet<N, S, C>> facets;

        std::array<int, N + 1> initial_vertices;

        create_initial_convex_hull(points, &initial_vertices, &facets);

        std::vector<unsigned char> point_enabled(points.size(), true);
        for (const int v : initial_vertices)
        {
                point_enabled[v] = false;
        }

        std::vector<FacetStorage<Facet<N, S, C>>> point_conflicts(points.size());

        create_initial_conflict_lists(points, point_enabled, &facets, &point_conflicts);

        ThreadPool thread_pool(thread_count_for_horizon<S, C>());
        std::barrier<> barrier(thread_pool.thread_count());

        std::vector<std::vector<signed char>> unique_points_work(thread_pool.thread_count());
        for (std::vector<signed char>& v : unique_points_work)
        {
                v.resize(points.size(), 0);
        }

        // Initial simplex created, so N + 1 points already processed
        for (std::size_t i = 0, points_processed = N + 1; i < points.size(); ++i, ++points_processed)
        {
                if (!point_enabled[i])
                {
                        continue;
                }

                if (progress::Ratio::lock_free())
                {
                        progress->set(points_processed, points.size());
                }

                add_point_to_convex_hull(
                        points, i, &facets, &point_conflicts, &thread_pool, &barrier, &unique_points_work);
        }

        ASSERT(std::all_of(
                facets.cbegin(), facets.cend(),
                [](const Facet<N, S, C>& facet)
                {
                        return facet.conflict_points().empty();
                }));

        return facets;
}
}

template <typename C, std::size_t N, typename S>
FacetList<Facet<N, S, C>> compute_convex_hull(
        const std::vector<numerical::Vector<N, S>>& points,
        progress::Ratio* const progress)
{
        return compute_implementation::compute_convex_hull<C>(points, progress);
}
}
