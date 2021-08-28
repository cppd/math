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

#include "convex_hull.h"

#include "facet.h"
#include "ridge.h"

#include <src/com/arrays.h>
#include <src/com/barrier.h>
#include <src/com/bits.h>
#include <src/com/combinatorics.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/thread_pool.h>
#include <src/com/type/find.h>
#include <src/com/type/limit.h>
#include <src/com/type/trait.h>
#include <src/numerical/determinant.h>
#include <src/numerical/difference.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace ns::geometry
{
namespace
{
constexpr int ORDINARY_BITS = 30;
constexpr int PARABOLOID_BITS = 24;

template <std::size_t N, std::size_t BITS>
constexpr int max_determinant_paraboloid()
{
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // |x x x x*x+x*x+x*x|
        // max = x * x * x * (x*x + x*x + x*x) * 4!
        // max = (x ^ (N + 1)) * (N - 1) * N!

        static_assert(N >= 2 && N <= 33);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (unsigned i = 2; i <= N; ++i)
        {
                f *= i;
        }

        f *= (N - 1);

        return BITS * (N + 1) + log_2(f) + 1;
}

template <std::size_t N, std::size_t BITS>
constexpr int max_determinant()
{
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // |x x x x|
        // max = x * x * x * x * 4!
        // max = (x ^ N) * N!

        static_assert(N >= 2 && N <= 34);
        static_assert(BITS > 0);

        unsigned __int128 f = 1;
        for (unsigned i = 2; i <= N; ++i)
        {
                f *= i;
        }

        return BITS * N + log_2(f) + 1;
}

template <std::size_t N, std::size_t BITS>
constexpr int max_paraboloid()
{
        // max = x*x + x*x + x*x
        // max = (x ^ 2) * (N - 1)

        return BITS * 2 + log_2(N - 1) + 1;
}

template <std::size_t N>
using ComputeTypeOrdinary = LeastSignedInteger<max_determinant<N, ORDINARY_BITS>()>;
template <std::size_t N>
using DataTypeOrdinary = LeastSignedInteger<ORDINARY_BITS>;

template <std::size_t N>
using ComputeTypeParaboloid = LeastSignedInteger<max_determinant_paraboloid<N, PARABOLOID_BITS>()>;
template <std::size_t N>
using DataTypeParaboloid = LeastSignedInteger<max_paraboloid<N, PARABOLOID_BITS>()>;
template <std::size_t N>
using ComputeTypeAfterParaboloid = LeastSignedInteger<max_determinant<N, PARABOLOID_BITS>()>;
template <std::size_t N>
using DataTypeAfterParaboloid = LeastSignedInteger<PARABOLOID_BITS>;

template <std::size_t N, bool CHECK_NON_CLASS>
struct CheckTypes final
{
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DataTypeOrdinary<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<ComputeTypeOrdinary<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DataTypeParaboloid<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<ComputeTypeParaboloid<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<DataTypeAfterParaboloid<N>>);
        static_assert(!CHECK_NON_CLASS || !std::is_class_v<ComputeTypeAfterParaboloid<N>>);
        static_assert(is_integral<DataTypeOrdinary<N>>);
        static_assert(is_integral<ComputeTypeOrdinary<N>>);
        static_assert(is_integral<DataTypeParaboloid<N>>);
        static_assert(is_integral<ComputeTypeParaboloid<N>>);
        static_assert(is_integral<DataTypeAfterParaboloid<N>>);
        static_assert(is_integral<ComputeTypeAfterParaboloid<N>>);
};
template struct CheckTypes<2, true>;
template struct CheckTypes<3, true>;
template struct CheckTypes<4, true>;
template struct CheckTypes<5, false>;
template struct CheckTypes<6, false>;

//

template <typename F>
using FacetList = std::list<F>;
template <typename F>
using FacetListConstIterator = typename FacetList<F>::const_iterator;
template <std::size_t N, typename DataType, typename ComputeType>
using Facet = FacetInteger<N, DataType, ComputeType, FacetListConstIterator>;

template <typename T>
std::string type_str() requires(!std::is_same_v<std::remove_cv_t<T>, mpz_class>)
{
        static_assert(is_integral<T>);
        return to_string(limits<T>::digits) + " bits";
}

template <typename T>
std::string type_str() requires std::is_same_v<std::remove_cv_t<T>, mpz_class>
{
        return "mpz_class";
}

// Here multithreading is not always faster.
// When points are inside a sphere, multithreading speeds up calculations.
// When points are on a sphere, multithreading slow down calculations.
// When using mpz_class, multithreading speeds up calculations.
template <typename S, typename C>
int thread_count_for_horizon()
{
        static_assert(is_integral<S> && is_integral<C>);

        if constexpr (!std::is_class_v<S> && !std::is_class_v<C>)
        {
                return 1;
        }
        else
        {
                const int hc = hardware_concurrency();
                return std::max(hc - 1, 1);
        }
}

template <typename T>
class FacetStore final
{
        // std::vector is faster than std::forward_list, std::list, std::set, std::unordered_set
        std::vector<const T*> data_;

public:
        void insert(const T* f)
        {
                data_.push_back(f);
        }
#if 0
        void erase(const T* f)
        {
                data_.erase(std::find(data_.cbegin(), data_.cend(), f));
        }
#endif
#if 0
        void erase(const T* f)
        {
                int_fast32_t size = data_.size();
                int_fast32_t i = 0;
                while (i != size && data_[i] != f)
                {
                        ++i;
                }
                if (i != size)
                {
                        std::memmove(&data_[i], &data_[i + 1], sizeof(f) * (size - (i + 1)));
                        data_.resize(size - 1);
                }
        }
#endif
        void erase(const T* f)
        {
                int size = data_.size();
                const T* next = nullptr;
                for (int_fast32_t i = size - 1; i >= 0; --i)
                {
                        const T* current = data_[i];
                        data_[i] = next;
                        if (current != f)
                        {
                                next = current;
                        }
                        else
                        {
                                data_.resize(size - 1);
                                return;
                        }
                }
                error("facet not found in facets of point");
        }

        std::size_t size() const
        {
                return data_.size();
        }

        void clear()
        {
                data_.clear();
                // data_.shrink_to_fit();
        }

        typename std::vector<const T*>::const_iterator begin() const
        {
                return data_.cbegin();
        }

        typename std::vector<const T*>::const_iterator end() const
        {
                return data_.cend();
        }
};

// The T range is for determinants only, not for Gram matrix
template <int COUNT, std::size_t N, typename T>
bool linearly_independent(const std::array<Vector<N, T>, N>& vectors)
{
        static_assert(is_integral<T>);
        static_assert(N > 1);
        static_assert(COUNT > 0 && COUNT <= N);

        for (const std::array<unsigned char, COUNT>& h_map : combinations<N, COUNT>())
        {
                if (numerical::determinant(vectors, SEQUENCE_UCHAR_ARRAY<COUNT>, h_map) != 0)
                {
                        return true;
                }
        }

        return false;
}

template <unsigned SIMPLEX_I, std::size_t N, typename SourceType, typename ComputeType>
void find_simplex_points(
        const std::vector<Vector<N, SourceType>>& points,
        std::array<int, N + 1>* simplex_points,
        std::array<Vector<N, ComputeType>, N>* simplex_vectors,
        unsigned point_i)
{
        static_assert(N > 1);
        static_assert(SIMPLEX_I <= N);

        for (; point_i < points.size(); ++point_i)
        {
                numerical::difference(
                        &(*simplex_vectors)[SIMPLEX_I - 1], points[point_i], points[(*simplex_points)[0]]);

                if (linearly_independent<SIMPLEX_I>(*simplex_vectors))
                {
                        break;
                }
        }

        if (point_i == points.size())
        {
                error("point " + to_string(SIMPLEX_I + 1) + " of " + to_string(N) + "-simplex not found");
        }

        (*simplex_points)[SIMPLEX_I] = point_i;

        if constexpr (SIMPLEX_I + 1 < N + 1)
        {
                find_simplex_points<SIMPLEX_I + 1>(points, simplex_points, simplex_vectors, point_i + 1);
        }
}

template <std::size_t N, typename SourceType, typename ComputeType>
void find_simplex_points(const std::vector<Vector<N, SourceType>>& points, std::array<int, N + 1>* simplex_points)
{
        static_assert(N > 1);

        if (points.empty())
        {
                error("0-simplex not found");
        }

        std::array<Vector<N, ComputeType>, N> simplex_vectors;

        (*simplex_points)[0] = 0;

        find_simplex_points<1>(points, simplex_points, &simplex_vectors, 1);
}

template <std::size_t N, typename Facet, template <typename...> typename Map>
void connect_facets(
        Facet* facet,
        int exclude_point,
        Map<Ridge<N>, std::tuple<Facet*, unsigned>>* search_map,
        int* ridge_count)
{
        const std::array<int, N>& vertices = facet->vertices();
        for (unsigned r = 0; r < N; ++r)
        {
                if (vertices[r] == exclude_point)
                {
                        // the horizon ridge, the facet is aleady connected to it
                        // when the facet was created
                        continue;
                }

                Ridge<N> ridge(del_elem(vertices, r));

                auto search_iter = search_map->find(ridge);
                if (search_iter == search_map->end())
                {
                        search_map->emplace(std::move(ridge), std::make_tuple(facet, r));
                }
                else
                {
                        Facet* link_facet = std::get<0>(search_iter->second);
                        unsigned link_r = std::get<1>(search_iter->second);

                        facet->set_link(r, link_facet);
                        link_facet->set_link(link_r, facet);

                        search_map->erase(search_iter);

                        ++(*ridge_count);
                }
        }
}

template <std::size_t N, typename S, typename C>
void create_init_convex_hull(
        const std::vector<Vector<N, S>>& points,
        std::array<int, N + 1>* vertices,
        FacetList<Facet<N, S, C>>* facets)
{
        find_simplex_points<N, S, C>(points, vertices);

        facets->clear();
        for (unsigned f = 0; f < N + 1; ++f)
        {
                facets->emplace_back(points, del_elem(*vertices, f), (*vertices)[f], nullptr);
                std::prev(facets->end())->set_iter(std::prev(facets->cend()));
        }

        constexpr int RIDGE_COUNT = binomial<N + 1, N - 1>();

        int ridges = 0;
        std::unordered_map<Ridge<N>, std::tuple<Facet<N, S, C>*, unsigned>> search_map(RIDGE_COUNT);
        for (Facet<N, S, C>& facet : *facets)
        {
                connect_facets(&facet, -1, &search_map, &ridges);
        }
        ASSERT(search_map.empty());
        ASSERT(ridges == RIDGE_COUNT);
}

template <typename Point, typename Facet>
void create_init_conflict_lists(
        const std::vector<Point>& points,
        const std::vector<unsigned char>& enabled,
        FacetList<Facet>* facets,
        std::vector<FacetStore<Facet>>* point_conflicts)
{
        for (Facet& facet : *facets)
        {
                for (unsigned point = 0; point < points.size(); ++point)
                {
                        if (enabled[point] && facet.visible_from_point(points, point))
                        {
                                (*point_conflicts)[point].insert(&facet);
                                facet.add_conflict_point(point);
                        }
                }
        }
}

template <typename Point, typename Facet>
void add_conflict_points_to_new_facet(
        const std::vector<Point>& points,
        int point,
        std::vector<signed char>* unique_points,
        const Facet* facet_0,
        const Facet* facet_1,
        Facet* new_facet)
{
        for (int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 1;

                if (p != point && new_facet->visible_from_point(points, p))
                {
                        new_facet->add_conflict_point(p);
                }
        }
        for (int p : facet_1->conflict_points())
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
        for (int p : facet_0->conflict_points())
        {
                (*unique_points)[p] = 0;
        }
}

template <typename Facet>
void erase_visible_facets_from_conflict_points(
        unsigned thread_id,
        unsigned thread_count,
        std::vector<FacetStore<Facet>>* point_conflicts,
        int point)
{
        for (const Facet* facet : (*point_conflicts)[point])
        {
                for (int p : facet->conflict_points())
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
        unsigned thread_id,
        unsigned thread_count,
        std::vector<FacetList<Facet>>* new_facets_vector,
        std::vector<FacetStore<Facet>>* point_conflicts)
{
        for (unsigned i = 0; i < new_facets_vector->size(); ++i)
        {
                for (const Facet& facet : (*new_facets_vector)[i])
                {
                        for (int p : facet.conflict_points())
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
        unsigned thread_id,
        unsigned thread_count,
        const std::vector<Point>& points,
        int point,
        const std::vector<FacetStore<Facet>>& point_conflicts,
        std::vector<std::vector<signed char>>* unique_points_work,
        std::vector<FacetList<Facet>>* new_facets_vector)
{
        ASSERT(new_facets_vector->size() == thread_count);
        ASSERT(unique_points_work->size() == thread_count);

        std::vector<signed char>* unique_points = &(*unique_points_work)[thread_id];
        FacetList<Facet>* new_facets = &(*new_facets_vector)[thread_id];

        new_facets->clear();

        unsigned ridge_count = 0;

        for (const Facet* facet : point_conflicts[point])
        {
                for (unsigned r = 0; r < facet->vertices().size(); ++r)
                {
                        Facet* link_facet = facet->link(r);

                        if (link_facet->marked_as_visible())
                        {
                                continue;
                        }
                        if (ridge_count != thread_id)
                        {
                                ++ridge_count;
                                continue;
                        }
                        ++ridge_count;
                        thread_id += thread_count;

                        int link_index = link_facet->find_link_index(facet);

                        new_facets->emplace_back(
                                points, set_elem(facet->vertices(), r, point), link_facet->vertices()[link_index],
                                link_facet);

                        Facet* new_facet = &(*std::prev(new_facets->end()));
                        new_facet->set_iter(std::prev(new_facets->cend()));

                        new_facet->set_link(new_facet->find_index_for_point(point), link_facet);
                        link_facet->set_link(link_index, new_facet);

                        add_conflict_points_to_new_facet(points, point, unique_points, facet, link_facet, new_facet);
                }
        }
}

template <std::size_t N, typename S, typename C>
void create_horizon_facets(
        unsigned thread_id,
        unsigned thread_count,
        const std::vector<Vector<N, S>>& points,
        int point,
        std::vector<FacetStore<Facet<N, S, C>>>* point_conflicts,
        std::vector<std::vector<signed char>>* unique_points_work,
        std::vector<FacetList<Facet<N, S, C>>>* new_facets_vector,
        Barrier* barrier)
{
        try
        {
                create_facets_for_point_and_horizon(
                        thread_id, thread_count, points, point, *point_conflicts, unique_points_work,
                        new_facets_vector);
        }
        catch (...)
        {
                barrier->wait();
                throw;
        }
        barrier->wait();

        // erase first, then add.
        // this reduces the amount of searching.
        erase_visible_facets_from_conflict_points(thread_id, thread_count, point_conflicts, point);
        add_new_facets_to_conflict_points(thread_id, thread_count, new_facets_vector, point_conflicts);
}

template <typename T>
unsigned calculate_facet_count(const std::vector<T>& facets)
{
        return std::accumulate(
                facets.cbegin(), facets.cend(), 0,
                [](unsigned a, const T& b)
                {
                        return a + b.size();
                });
}

template <std::size_t N, typename S, typename C>
void add_point_to_convex_hull(
        const std::vector<Vector<N, S>>& points,
        int point,
        FacetList<Facet<N, S, C>>* facets,
        std::vector<FacetStore<Facet<N, S, C>>>* point_conflicts,
        ThreadPool* thread_pool,
        Barrier* barrier,
        std::vector<std::vector<signed char>>* unique_points_work)
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

        for (const Facet<N, S, C>* facet : (*point_conflicts)[point])
        {
                facet->mark_as_visible();
        }

        std::vector<FacetList<Facet<N, S, C>>> new_facets(thread_pool->thread_count());

        if (thread_pool->thread_count() > 1)
        {
                thread_pool->run(
                        [&](unsigned thread_id, unsigned thread_count)
                        {
                                create_horizon_facets(
                                        thread_id, thread_count, points, point, point_conflicts, unique_points_work,
                                        &new_facets, barrier);
                        });
        }
        else
        {
                // 0 = thread_id, 1 = thread_count
                create_horizon_facets(0, 1, points, point, point_conflicts, unique_points_work, &new_facets, barrier);
        }

        // Erase visible facets
        for (const Facet<N, S, C>* facet : (*point_conflicts)[point])
        {
                facets->erase(facet->iter());
        }

        (*point_conflicts)[point].clear();

        int facet_count = calculate_facet_count(new_facets);
        int ridge_count = (N - 1) * facet_count / 2;

        // Connect facets, excluding horizon facets
        int ridges = 0;
        std::unordered_map<Ridge<N>, std::tuple<Facet<N, S, C>*, unsigned>> search_map(ridge_count);
        for (unsigned i = 0; i < new_facets.size(); ++i)
        {
                for (Facet<N, S, C>& facet : new_facets[i])
                {
                        connect_facets(&facet, point, &search_map, &ridges);
                }
        }
        ASSERT(search_map.empty());
        ASSERT(ridges == ridge_count);

        for (unsigned i = 0; i < new_facets.size(); ++i)
        {
                facets->splice(facets->cend(), new_facets[i]);
        }
}

template <std::size_t N, typename S, typename C>
void create_convex_hull(
        const std::vector<Vector<N, S>>& points,
        FacetList<Facet<N, S, C>>* facets,
        ProgressRatio* progress)
{
        static_assert(N > 1);

        if (points.size() < N + 1)
        {
                error("Error point count " + to_string(points.size()) + " for convex hull in " + space_name(N));
        }

        facets->clear();

        std::array<int, N + 1> init_vertices;

        create_init_convex_hull(points, &init_vertices, facets);

        std::vector<unsigned char> point_enabled(points.size(), true);
        for (int v : init_vertices)
        {
                point_enabled[v] = false;
        }

        std::vector<FacetStore<Facet<N, S, C>>> point_conflicts(points.size());

        create_init_conflict_lists(points, point_enabled, facets, &point_conflicts);

        ThreadPool thread_pool(thread_count_for_horizon<S, C>());
        Barrier barrier(thread_pool.thread_count());

        std::vector<std::vector<signed char>> unique_points_work(thread_pool.thread_count());
        for (std::vector<signed char>& v : unique_points_work)
        {
                v.resize(points.size(), 0);
        }

        // Initial simplex created, so N + 1 points already processed
        for (unsigned i = 0, points_processed = N + 1; i < points.size(); ++i, ++points_processed)
        {
                if (!point_enabled[i])
                {
                        continue;
                }

                if (ProgressRatio::lock_free())
                {
                        progress->set(points_processed, points.size());
                }

                add_point_to_convex_hull(
                        points, i, facets, &point_conflicts, &thread_pool, &barrier, &unique_points_work);
        }

        ASSERT(std::all_of(
                facets->cbegin(), facets->cend(),
                [](const Facet<N, S, C>& facet) -> bool
                {
                        return facet.conflict_points().empty();
                }));
}

template <std::size_t N>
void find_min_max(const std::vector<Vector<N, float>>& points, Vector<N, float>* min, Vector<N, float>* max)
{
        ASSERT(!points.empty());

        *min = *max = points[0];
        for (unsigned i = 1; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        (*min)[n] = std::min((*min)[n], points[i][n]);
                        (*max)[n] = std::max((*max)[n], points[i][n]);
                }
        }
}

template <std::size_t N>
void shuffle_and_convert_to_unique_integer(
        const std::vector<Vector<N, float>>& source_points,
        long long max_value,
        std::vector<Vector<N, long long>>* points,
        std::vector<int>* points_map)
{
        ASSERT(0 < max_value);

        points->clear();
        points->reserve(source_points.size());

        points_map->clear();
        points_map->reserve(source_points.size());

        std::vector<int> random_map(source_points.size());
        std::iota(random_map.begin(), random_map.end(), 0);
        std::shuffle(random_map.begin(), random_map.end(), std::mt19937_64(source_points.size()));

        Vector<N, float> min;
        Vector<N, float> max;
        find_min_max(source_points, &min, &max);

        double max_d = (max - min).norm_infinity();
        if (max_d == 0)
        {
                error("all points are equal to each other");
        }
        double scale_factor = max_value / max_d;

        std::unordered_set<Vector<N, long long>> unique_check(source_points.size());

        for (unsigned i = 0; i < source_points.size(); ++i)
        {
                int random_i = random_map[i];

                Vector<N, double> float_value = to_vector<double>(source_points[random_i] - min) * scale_factor;

                Vector<N, long long> integer_value;

                for (unsigned n = 0; n < N; ++n)
                {
                        long long ll = std::llround(float_value[n]);
                        if (ll < 0 || ll > max_value)
                        {
                                error("points preprocessing error: value < 0 || value > MAX");
                        }
                        integer_value[n] = ll;
                }

                if (unique_check.count(integer_value) == 0)
                {
                        unique_check.insert(integer_value);
                        points->push_back(integer_value);
                        points_map->push_back(random_i);
                }
        }
}

template <std::size_t N>
std::array<int, N> restore_indices(const std::array<int, N>& vertices, const std::vector<int>& points_map)
{
        std::array<int, N> res;
        for (unsigned n = 0; n < N; ++n)
        {
                res[n] = points_map[vertices[n]];
        }
        return res;
}

template <std::size_t N>
void paraboloid_convex_hull(
        const std::vector<Vector<N, long long>>& points,
        const std::vector<int>& points_map,
        std::vector<DelaunaySimplex<N>>* simplices,
        ProgressRatio* progress,
        bool write_log)
{
        using FacetCH = Facet<N + 1, DataTypeParaboloid<N + 1>, ComputeTypeParaboloid<N + 1>>;
        using PointCH = Vector<N + 1, DataTypeParaboloid<N + 1>>;

        if (write_log)
        {
                std::ostringstream oss;
                oss << "Convex hull" << '\n';
                oss << "  Paraboloid " << space_name(N + 1) << '\n';
                oss << "    Max: " << PARABOLOID_BITS << '\n';
                oss << "    Data: " << type_str<DataTypeParaboloid<N + 1>>() << '\n';
                oss << "    Compute: " << type_str<ComputeTypeParaboloid<N + 1>>() << '\n';
                oss << "  " << space_name(N) << '\n';
                oss << "    Data: " << type_str<DataTypeAfterParaboloid<N>>() << '\n';
                oss << "    Compute: " << type_str<ComputeTypeAfterParaboloid<N>>();
                LOG(oss.str());
        }

        std::vector<PointCH> data(points.size());

        // place points onto paraboloid
        for (unsigned i = 0; i < points.size(); ++i)
        {
                data[i][N] = 0;
                for (unsigned n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                        // multipication using data type of the 'data'
                        data[i][N] += data[i][n] * data[i][n];
                }
        }

        FacetList<FacetCH> facets;

        create_convex_hull(data, &facets, progress);

        data.clear();
        data.shrink_to_fit();

        // compute ortho in n-space and create facets

        using FacetDelaunay = Facet<N, DataTypeAfterParaboloid<N>, ComputeTypeAfterParaboloid<N>>;
        using PointDelaunay = Vector<N, DataTypeAfterParaboloid<N>>;

        std::vector<PointDelaunay> data_d(points.size());
        for (unsigned i = 0; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        data_d[i][n] = points[i][n];
                }
        }

        simplices->clear();
        simplices->reserve(facets.size());
        for (const FacetCH& facet : facets)
        {
                if (!facet.last_ortho_coord_is_negative())
                {
                        // not the lower convex hull
                        continue;
                }

                const std::array<int, N + 1>& vertices = facet.vertices();

                std::array<Vector<N, double>, N + 1> orthos;
                for (unsigned r = 0; r < N + 1; ++r)
                {
                        // ortho is directed outside
                        orthos[r] = FacetDelaunay(data_d, del_elem(vertices, r), vertices[r], nullptr).double_ortho();
                }

                simplices->emplace_back(restore_indices(vertices, points_map), orthos);
        }
}

template <std::size_t N>
void ordinary_convex_hull(
        const std::vector<Vector<N, long long>>& points,
        const std::vector<int>& points_map,
        std::vector<ConvexHullFacet<N>>* ch_facets,
        ProgressRatio* progress,
        bool write_log)
{
        using Facet = Facet<N, DataTypeOrdinary<N>, ComputeTypeOrdinary<N>>;
        using Point = Vector<N, DataTypeOrdinary<N>>;

        if (write_log)
        {
                std::ostringstream oss;
                oss << "Convex hull " << space_name(N) << '\n';
                oss << "  Max: " << to_string(ORDINARY_BITS) << '\n';
                oss << "  Data: " << type_str<DataTypeOrdinary<N>>() << '\n';
                oss << "  Compute: " << type_str<ComputeTypeOrdinary<N>>();
                LOG(oss.str());
        }

        std::vector<Point> data(points.size());

        for (unsigned i = 0; i < points.size(); ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        data[i][n] = points[i][n];
                }
        }

        FacetList<Facet> facets;

        create_convex_hull(data, &facets, progress);

        ch_facets->clear();
        ch_facets->reserve(facets.size());
        for (const Facet& facet : facets)
        {
                ch_facets->emplace_back(restore_indices(facet.vertices(), points_map), facet.double_ortho());
        }
}

//

template <std::size_t N>
void delaunay_integer(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<Vector<N, double>>* points,
        std::vector<DelaunaySimplex<N>>* simplices,
        ProgressRatio* progress,
        bool write_log)
{
        if (write_log)
        {
                LOG("Convex hull paraboloid in " + space_name(N + 1) + " integer");
        }

        std::vector<Vector<N, long long>> convex_hull_points;
        std::vector<int> points_map;

        constexpr long long MAX = (1ull << PARABOLOID_BITS) - 1;

        shuffle_and_convert_to_unique_integer(source_points, MAX, &convex_hull_points, &points_map);

        paraboloid_convex_hull(convex_hull_points, points_map, simplices, progress, write_log);

        points->clear();
        points->resize(source_points.size(), Vector<N, double>(0));
        for (unsigned i = 0; i < convex_hull_points.size(); ++i)
        {
                (*points)[points_map[i]] = to_vector<double>(convex_hull_points[i]);
        }

        if (write_log)
        {
                LOG("Convex hull paraboloid in " + space_name(N + 1) + " integer done");
        }
}

template <std::size_t N>
void convex_hull_integer(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<ConvexHullFacet<N>>* facets,
        ProgressRatio* progress,
        bool write_log)
{
        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer");
        }

        std::vector<int> points_map;
        std::vector<Vector<N, long long>> convex_hull_points;

        constexpr long long MAX = (1ull << ORDINARY_BITS) - 1;

        shuffle_and_convert_to_unique_integer(source_points, MAX, &convex_hull_points, &points_map);

        ordinary_convex_hull(convex_hull_points, points_map, facets, progress, write_log);

        if (write_log)
        {
                LOG("Convex hull in " + space_name(N) + " integer done");
        }
}
}

template <std::size_t N>
void compute_delaunay(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<Vector<N, double>>* points,
        std::vector<DelaunaySimplex<N>>* simplices,
        ProgressRatio* progress,
        bool write_log)
{
        if (source_points.empty())
        {
                error("No points for compute delaunay");
        }

        delaunay_integer(source_points, points, simplices, progress, write_log);
}

template <std::size_t N>
void compute_convex_hull(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<ConvexHullFacet<N>>* ch_facets,
        ProgressRatio* progress,
        bool write_log)
{
        if (source_points.empty())
        {
                error("No points for compute convex hull");
        }

        convex_hull_integer(source_points, ch_facets, progress, write_log);
}

#define COMPUTE_DELAUNAY_INSTANTIATION(N)                                                  \
        template void compute_delaunay(                                                    \
                const std::vector<Vector<(N), float>>&, std::vector<Vector<(N), double>>*, \
                std::vector<DelaunaySimplex<(N)>>*, ProgressRatio*, bool);

#define COMPUTE_CONVEX_HULL_INSTANTIATION(N) \
        template void compute_convex_hull(   \
                const std::vector<Vector<(N), float>>&, std::vector<ConvexHullFacet<(N)>>*, ProgressRatio*, bool);

COMPUTE_DELAUNAY_INSTANTIATION(2)
COMPUTE_DELAUNAY_INSTANTIATION(3)
COMPUTE_DELAUNAY_INSTANTIATION(4)
COMPUTE_DELAUNAY_INSTANTIATION(5)

COMPUTE_CONVEX_HULL_INSTANTIATION(2)
COMPUTE_CONVEX_HULL_INSTANTIATION(3)
COMPUTE_CONVEX_HULL_INSTANTIATION(4)
COMPUTE_CONVEX_HULL_INSTANTIATION(5)
COMPUTE_CONVEX_HULL_INSTANTIATION(6)
}
