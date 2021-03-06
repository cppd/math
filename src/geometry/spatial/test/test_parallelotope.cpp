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

#include "../hyperplane_parallelotope.h"
#include "../parallelotope.h"
#include "../parallelotope_aa.h"
#include "../shape_intersection.h"
#include "../shape_wrapper.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/test/test.h>

#include <algorithm>
#include <memory>
#include <random>
#include <utility>

namespace ns::geometry
{
namespace
{
constexpr bool PRINT_ALL = false;

constexpr int POINT_COUNT = 10000;

template <typename T>
constexpr T POSITION_DELTA;
template <>
constexpr double POSITION_DELTA<double> = 1e-6;

template <typename T>
constexpr T EQUALITY_EPSILON;
template <>
constexpr double EQUALITY_EPSILON<double> = 1e-10;

template <typename T>
constexpr T MAX_DOT_PRODUCT_OF_EDGES;
template <>
constexpr double MAX_DOT_PRODUCT_OF_EDGES<double> = 0.9;

template <typename Parallelotope>
using VectorP = Vector<Parallelotope::SPACE_DIMENSION, typename Parallelotope::DataType>;

template <typename T, std::size_t... I>
constexpr Vector<sizeof...(I), T> last_coord_impl(T v, T last, std::integer_sequence<std::size_t, I...>&&)
{
        return Vector<sizeof...(I), T>((I + 1 < sizeof...(I) ? v : last)...);
}

template <std::size_t N, typename T>
constexpr Vector<N, T> last_coord(T v, T last)
{
        return last_coord_impl(v, last, std::make_integer_sequence<std::size_t, N>());
}

void print_separator()
{
        if (PRINT_ALL)
        {
                LOG("---");
        }
}

void print_message(const std::string& msg)
{
        if (PRINT_ALL)
        {
                LOG(msg);
        }
}

template <typename T>
bool almost_equal(T a, T b)
{
        return std::abs(a - b) <= EQUALITY_EPSILON<T>;
}

template <std::size_t N, typename T>
bool almost_equal(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return (a - b).norm() <= EQUALITY_EPSILON<T>;
}

template <std::size_t N, typename T>
bool test_edges(T edge_min_length, T edge_max_length, const std::array<Vector<N, T>, N>& edges)
{
        std::array<Vector<N, T>, N> unit_edges = edges;
        for (Vector<N, T>& v : unit_edges)
        {
                T length = v.norm();
                if (!(length >= edge_min_length && length <= edge_max_length))
                {
                        return false;
                }
                v /= length;
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        if (!(std::abs(dot(unit_edges[i], unit_edges[j])) < MAX_DOT_PRODUCT_OF_EDGES<T>))
                        {
                                return false;
                        }
                }
        }

        return true;
}

template <std::size_t N, typename T, typename Engine>
std::array<Vector<N, T>, N> generate_edges(T edge_min_length, T edge_max_length, Engine& engine)
{
        ASSERT(edge_min_length > 0 && edge_min_length <= edge_max_length);

        std::uniform_real_distribution<T> urd(-edge_max_length, edge_max_length);

        std::array<Vector<N, T>, N> edges;
        do
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = 0; j < N; ++j)
                        {
                                edges[i][j] = urd(engine);
                        }
                }

        } while (!test_edges(edge_min_length, edge_max_length, edges));
        return edges;
}

template <std::size_t N, typename T, typename Engine>
std::array<T, N> generate_aa_edges(T edge_min_length, T edge_max_length, Engine& engine)
{
        ASSERT(edge_min_length > 0 && edge_min_length <= edge_max_length);

        std::uniform_real_distribution<T> urd(edge_min_length, edge_max_length);
        std::array<T, N> edges;
        for (std::size_t i = 0; i < N; ++i)
        {
                edges[i] = urd(engine);
        }
        return edges;
}

template <std::size_t N, typename T, typename Engine>
Vector<N, T> generate_org(T org_size, Engine& engine)
{
        ASSERT(org_size >= 0);

        std::uniform_real_distribution<T> urd(-org_size, org_size);
        Vector<N, T> org;
        for (std::size_t i = 0; i < N; ++i)
        {
                org[i] = urd(engine);
        }
        return org;
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<VectorP<Parallelotope>> external_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> low = {
                std::uniform_real_distribution<T>{-len[I] * 10, -POSITION_DELTA<T> * len[I]}...};
        std::array<std::uniform_real_distribution<T>, N> high = {
                std::uniform_real_distribution<T>{len[I] * (1 + POSITION_DELTA<T>), len[I] * 10}...};

        std::uniform_int_distribution<int> rnd(0, 1);

        std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::vector<Vector<N, T>> points;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((rnd(engine) ? low[I](engine) : high[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<VectorP<Parallelotope>> internal_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> internal = {
                std::uniform_real_distribution<T>{len[I] * POSITION_DELTA<T>, len[I] * (1 - POSITION_DELTA<T>)}...};

        std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::vector<Vector<N, T>> points;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((internal[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, std::size_t... I>
std::vector<VectorP<Parallelotope>> cover_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<std::size_t, I...>)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {p.e(I).norm()...};

        std::array<std::uniform_real_distribution<T>, N> cover = {
                std::uniform_real_distribution<T>{static_cast<T>(-0.2) * len[I], len[I] * static_cast<T>(1.2)}...};

        std::array<std::uniform_real_distribution<T>, N> len_random = {std::uniform_real_distribution<T>{0, len[I]}...};

        std::array<Vector<N, T>, N> unit = {(p.e(I) / len[I])...};

        std::vector<Vector<N, T>> points;

        for (int i = 0; i < count; ++i)
        {
                // Точки на всё пространство параллелотопа с запасом
                points.push_back(p.org() + ((unit[I] * cover[I](engine)) + ...));

                // Точки на гранях параллелотопа
                for (unsigned n = 0; n < N; ++n)
                {
                        Vector<N, T> v;

                        v = p.org();
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_random[d](engine);
                                }
                        }
                        points.push_back(v);

                        v = p.org();
                        for (unsigned d = 0; d < N; ++d)
                        {
                                if (d != n)
                                {
                                        v += unit[d] * len_random[d](engine);
                                }
                        }
                        points.push_back(v + p.e(n));
                }
        }

        return points;
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd_dir(-1, 1);

        while (true)
        {
                Vector<N, T> direction;

                // Равновероятность всех направлений не нужна
                for (unsigned i = 0; i < N; ++i)
                {
                        direction[i] = urd_dir(engine);
                }

                if (direction.norm() > 0)
                {
                        return direction;
                }
        }
}

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction_for_parallelotope_comparison(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd_dir(-1, 1);
        std::uniform_int_distribution<int> uid_dir(-1, 1);
        std::uniform_int_distribution<int> uid_select(0, 10);

        while (true)
        {
                Vector<N, T> direction;

                // Равновероятность всех направлений не нужна
                for (unsigned i = 0; i < N; ++i)
                {
                        direction[i] = uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine);
                }

                if (direction.norm() > 0)
                {
                        return direction;
                }
        }
}

template <std::size_t N, std::size_t Count, typename T>
bool point_is_in_feasible_region(const Vector<N, T>& point, const std::array<Constraint<N, T>, Count>& c)
{
        for (unsigned i = 0; i < Count; ++i)
        {
                T r = dot(c[i].a, point) + c[i].b;

                if (!is_finite(r))
                {
                        error("Not finite point " + to_string(point) + " and constraint a = " + to_string(c[i].a)
                              + ", b = " + to_string(c[i].b));
                }

                if (r < 0)
                {
                        return false;
                }
        }

        return true;
}

template <typename RandomEngine, typename Parallelotope>
void test_points(RandomEngine& engine, int point_count, const Parallelotope& p)
{
        constexpr std::size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        T length = p.length();

        const Constraints<N, T, 2 * N, 0> constraints = p.constraints();

        for (const Vector<N, T>& point :
             external_points(engine, point_count, p, std::make_integer_sequence<std::size_t, N>()))
        {
                if (p.inside(point))
                {
                        error("Inside. Point must be outside\n" + to_string(point));
                }

                if (point_is_in_feasible_region(point, constraints.c))
                {
                        error("Constraints. Point must be outside\n" + to_string(point));
                }
        }

        for (const Vector<N, T>& origin :
             internal_points(engine, point_count, p, std::make_integer_sequence<std::size_t, N>()))
        {
                if (!p.inside(origin))
                {
                        error("Inside. Point must be inside\n" + to_string(origin));
                }

                if (!point_is_in_feasible_region(origin, constraints.c))
                {
                        error("Constraints. Point must be inside\n" + to_string(origin));
                }

                Vector<N, T> direction = random_direction<N, T>(engine);

                Ray<N, T> ray_orig(origin, direction);

                std::optional<T> t;
                Ray<N, T> ray;

                ray = ray_orig;
                t = p.intersect(ray);
                if (!t)
                {
                        error("Ray must intersect\n" + to_string(ray));
                }
                if (*t >= length)
                {
                        error("Intersection out of parallelotope.\ndistance = " + to_string(*t) + ", "
                              + "max distance = " + to_string(length) + "\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * length), direction);
                t = p.intersect(ray);
                if (!t)
                {
                        error("Ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * length), -direction);
                t = p.intersect(ray);
                if (!t)
                {
                        error("Ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * length), direction);
                t = p.intersect(ray);
                if (t)
                {
                        error("Ray must not intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * length), -direction);
                t = p.intersect(ray);
                if (t)
                {
                        error("Ray must not intersect\n" + to_string(ray));
                }
        }
}

template <std::size_t N, typename T, typename... Parallelotope>
void verify_intersection(const Ray<N, T>& ray, const Parallelotope&... p)
{
        static_assert(((N == Parallelotope::SPACE_DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<std::optional<T>, sizeof...(Parallelotope)> intersections{p.intersect(ray)...};

        for (unsigned i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (intersections[i].has_value() != intersections[0].has_value())
                {
                        error("Error intersection comparison\n" + to_string(ray));
                }
                if (!intersections[i])
                {
                        continue;
                }
                if (!almost_equal(*intersections[i], *intersections[0]))
                {
                        std::string s = "Error intersection distance comparison.\n";
                        s += "Distance[" + to_string(i) + "] = " + to_string(*intersections[i]) + "\n";
                        s += "Distance[0] = " + to_string(*intersections[0]) + "\n";
                        s += "Ray = " + to_string(ray);
                        error(s);
                }
        }
}

template <std::size_t N, typename T, std::size_t Count>
void verify_vectors(const std::array<Vector<N, T>, Count>& vectors, const std::string& name)
{
        for (unsigned i = 1; i < Count; ++i)
        {
                if (!almost_equal(vectors[i], vectors[0]))
                {
                        error("Error comparison of " + name + ".\n" + to_string(vectors[i]) + " and "
                              + to_string(vectors[0]));
                }
        }
}

template <typename RandomEngine, typename... Parallelotope>
void compare_parallelotopes(RandomEngine& engine, int point_count, const Parallelotope&... p)
{
        static_assert(sizeof...(Parallelotope) >= 2);

        constexpr std::size_t N = std::get<0>(std::make_tuple(Parallelotope::SPACE_DIMENSION...));
        using T = typename std::tuple_element_t<0, std::tuple<Parallelotope...>>::DataType;

        static_assert(((N == Parallelotope::SPACE_DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<T, sizeof...(Parallelotope)> lengths{p.length()...};

        for (unsigned i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (!almost_equal(lengths[i], lengths[0]))
                {
                        error("Error diagonal max length.\n" + to_string(lengths[i]) + " and " + to_string(lengths[0]));
                }
        }

        std::array<Vector<N, T>, sizeof...(Parallelotope)> orgs{p.org()...};
        verify_vectors(orgs, "orgs");

        for (unsigned i = 0; i < N; ++i)
        {
                std::array<Vector<N, T>, sizeof...(Parallelotope)> e{p.e(i)...};
                verify_vectors(e, "e" + to_string(i));
        }

        for (Vector<N, T> origin : cover_points(
                     engine, point_count, std::get<0>(std::make_tuple(p...)),
                     std::make_integer_sequence<std::size_t, N>()))
        {
                std::array<bool, sizeof...(Parallelotope)> inside{p.inside(origin)...};
                for (unsigned i = 1; i < sizeof...(Parallelotope); ++i)
                {
                        if (inside[i] != inside[0])
                        {
                                error("Error point inside\n" + to_string(origin));
                        }
                }

                Vector<N, T> direction = random_direction_for_parallelotope_comparison<N, T>(engine);

                Ray<N, T> ray_orig(origin, direction);

                Ray<N, T> ray;

                ray = Ray<N, T>(ray_orig.org(), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(-10 * lengths[0]), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(10 * lengths[0]), -direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(10 * lengths[0]), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(-10 * lengths[0]), -direction);

                verify_intersection(ray, p...);
        }
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N, T>, N> to_edge_vector(const std::array<T, N>& edges)
{
        std::array<Vector<N, T>, N> edge_vector;
        for (unsigned i = 0; i < N; ++i)
        {
                for (unsigned j = 0; j < N; ++j)
                {
                        edge_vector[i][j] = (i != j) ? 0 : edges[i];
                }
        }
        return edge_vector;
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N + 1, T>, N> to_edge_vector_hyper(const std::array<T, N>& edges)
{
        std::array<Vector<N + 1, T>, N> edge_vector;
        for (unsigned i = 0; i < N; ++i)
        {
                for (unsigned j = 0; j < N + 1; ++j)
                {
                        edge_vector[i][j] = (i != j) ? 0 : edges[i];
                }
        }
        return edge_vector;
}

template <std::size_t N, typename T>
void test_points(int point_count)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        constexpr T ORG_SIZE = 10;
        constexpr T MIN_LENGTH = 0.1;
        constexpr T MAX_LENGTH = 20;

        LOG("------------------------------");
        LOG("Parallelotope points in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                Vector<N, T> org = generate_org<N, T>(ORG_SIZE, engine);
                std::array<T, N> edges = generate_aa_edges<N, T>(MIN_LENGTH, MAX_LENGTH, engine);
                ParallelotopeAA<N, T> p(org, edges);

                print_message(to_string(p));

                test_points(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Vector<N, T> org = generate_org<N, T>(ORG_SIZE, engine);
                std::array<Vector<N, T>, N> edges = generate_edges<N, T>(MIN_LENGTH, MAX_LENGTH, engine);
                Parallelotope<N, T> p(org, edges);

                print_message(to_string(p));

                test_points(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope comparison");

        {
                Vector<N, T> org = generate_org<N, T>(ORG_SIZE, engine);
                std::array<T, N> edges = generate_aa_edges<N, T>(MIN_LENGTH, MAX_LENGTH, engine);

                ParallelotopeAA<N, T> p_aa(org, edges);
                Parallelotope<N, T> p(org, to_edge_vector(edges));

                print_message("#1\n" + to_string(p_aa) + "\n#2\n" + to_string(p));

                compare_parallelotopes(engine, point_count, p_aa, p);
        }

        print_separator();
        LOG("Check passed");
}

template <typename Parallelotope>
void test_algorithms(const Parallelotope& p)
{
        print_separator();
        print_message("length");
        print_message(to_string(p.length()));

        print_separator();
        print_message("vertices");
        for (auto v : p.vertices())
        {
                print_message(to_string(v));
        }

        if constexpr (Parallelotope::SPACE_DIMENSION <= 3)
        {
                print_separator();
                print_message("edges");
                for (auto edge : p.edges())
                {
                        print_message(to_string(edge));
                }
        }
}

template <std::size_t N, typename T>
void test_algorithms()
{
        constexpr std::array<T, N> EDGES = make_array_value<T, N>(1);
        constexpr Vector<N, T> ORG(0);

        LOG("------------------------------");
        LOG("Parallelotope algorithms in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p(ORG, EDGES);
                test_algorithms(p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Parallelotope<N, T> p(ORG, to_edge_vector(EDGES));
                test_algorithms(p);
        }

        print_separator();
        LOG("Check passed");
}

template <typename Parallelotope1, typename Parallelotope2>
void test_intersection(
        const Parallelotope1& p1,
        const Parallelotope2& p2,
        bool with_intersection,
        const std::string& text)
{
        if (with_intersection != shape_intersection(p1, p2))
        {
                error("Error intersection " + text);
        }

        print_message("intersection " + text);
}

template <typename Parallelotope>
std::unique_ptr<ShapeWrapperForIntersection<Parallelotope>> make_unique_wrapper(const Parallelotope& p)
{
        return std::make_unique<ShapeWrapperForIntersection<Parallelotope>>(p);
}

template <std::size_t N, typename T>
void test_intersections()
{
        constexpr std::array<T, N> EDGES = make_array_value<T, N>(1);
        constexpr Vector<N, T> ORG_0(0.0);
        constexpr Vector<N, T> ORG_1(0.75);
        constexpr Vector<N, T> ORG_2(1.5);

        constexpr Vector<N, T> ORG_BIG(-5);
        constexpr std::array<T, N> EDGES_BIG = make_array_value<T, N>(10);

        LOG("------------------------------");
        LOG("Parallelotope intersections in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p1(ORG_0, EDGES);
                ParallelotopeAA<N, T> p2(ORG_1, EDGES);
                ParallelotopeAA<N, T> p3(ORG_2, EDGES);
                ParallelotopeAA<N, T> p_big(ORG_BIG, EDGES_BIG);

                std::unique_ptr w1 = make_unique_wrapper(p1);
                std::unique_ptr w2 = make_unique_wrapper(p2);
                std::unique_ptr w3 = make_unique_wrapper(p3);
                std::unique_ptr w_big = make_unique_wrapper(p_big);

                test_intersection(*w1, *w2, true, "1-2");
                test_intersection(*w2, *w3, true, "2-3");
                test_intersection(*w1, *w3, false, "1-3");

                test_intersection(*w1, *w_big, true, "1-big");
                test_intersection(*w2, *w_big, true, "2-big");
                test_intersection(*w3, *w_big, true, "3-big");
        }

        print_separator();
        LOG("Parallelotope");

        {
                Parallelotope<N, T> p1(ORG_0, to_edge_vector(EDGES));
                Parallelotope<N, T> p2(ORG_1, to_edge_vector(EDGES));
                Parallelotope<N, T> p3(ORG_2, to_edge_vector(EDGES));
                Parallelotope<N, T> p_big(ORG_BIG, to_edge_vector(EDGES_BIG));

                std::unique_ptr w1 = make_unique_wrapper(p1);
                std::unique_ptr w2 = make_unique_wrapper(p2);
                std::unique_ptr w3 = make_unique_wrapper(p3);
                std::unique_ptr w_big = make_unique_wrapper(p_big);

                test_intersection(*w1, *w2, true, "1-2");
                test_intersection(*w2, *w3, true, "2-3");
                test_intersection(*w1, *w3, false, "1-3");

                test_intersection(*w1, *w_big, true, "1-big");
                test_intersection(*w2, *w_big, true, "2-big");
                test_intersection(*w3, *w_big, true, "3-big");
        }

        print_separator();
        LOG("Check passed");
}

template <std::size_t N, typename T>
void test_intersections_hyperplane()
{
        constexpr Vector<N, T> ORG(5);
        constexpr T SIZE = 1;

        constexpr T SIZE_BIG = 3;

        constexpr Vector<N, T> BIG_1 = last_coord<N, T>(4.0, 4.9);
        constexpr Vector<N, T> BIG_2 = last_coord<N, T>(4.0, 5.5);
        constexpr Vector<N, T> BIG_3 = last_coord<N, T>(4.0, 6.1);

        constexpr T SIZE_SMALL = 0.2;

        constexpr Vector<N, T> SMALL_1 = last_coord<N, T>(4.9, 4.9);
        constexpr Vector<N, T> SMALL_2 = last_coord<N, T>(4.9, 5.5);
        constexpr Vector<N, T> SMALL_3 = last_coord<N, T>(4.9, 6.1);

        constexpr Vector<N, T> SMALL_4 = last_coord<N, T>(4.0, 4.9);
        constexpr Vector<N, T> SMALL_5 = last_coord<N, T>(4.0, 5.5);
        constexpr Vector<N, T> SMALL_6 = last_coord<N, T>(4.0, 6.1);

        constexpr Vector<N, T> SMALL_7 = last_coord<N, T>(5.5, 4.9);
        constexpr Vector<N, T> SMALL_8 = last_coord<N, T>(5.5, 5.5);
        constexpr Vector<N, T> SMALL_9 = last_coord<N, T>(5.5, 6.1);

        LOG("------------------------------");
        LOG("Hyperplane parallelotope intersections in " + space_name(N));

        constexpr std::array<Vector<N, T>, N - 1> EDGES_HYPER_BIG =
                to_edge_vector_hyper(make_array_value<T, N - 1>(SIZE_BIG));

        constexpr std::array<Vector<N, T>, N - 1> EDGES_HYPER_SMALL =
                to_edge_vector_hyper(make_array_value<T, N - 1>(SIZE_SMALL));

        HyperplaneParallelotope<N, T> p1(BIG_1, EDGES_HYPER_BIG);
        HyperplaneParallelotope<N, T> p2(BIG_2, EDGES_HYPER_BIG);
        HyperplaneParallelotope<N, T> p3(BIG_3, EDGES_HYPER_BIG);
        HyperplaneParallelotope<N, T> p4(SMALL_1, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p5(SMALL_2, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p6(SMALL_3, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p7(SMALL_4, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p8(SMALL_5, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p9(SMALL_6, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p10(SMALL_7, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p11(SMALL_8, EDGES_HYPER_SMALL);
        HyperplaneParallelotope<N, T> p12(SMALL_9, EDGES_HYPER_SMALL);

        std::unique_ptr w1 = make_unique_wrapper(p1);
        std::unique_ptr w2 = make_unique_wrapper(p2);
        std::unique_ptr w3 = make_unique_wrapper(p3);
        std::unique_ptr w4 = make_unique_wrapper(p4);
        std::unique_ptr w5 = make_unique_wrapper(p5);
        std::unique_ptr w6 = make_unique_wrapper(p6);
        std::unique_ptr w7 = make_unique_wrapper(p7);
        std::unique_ptr w8 = make_unique_wrapper(p8);
        std::unique_ptr w9 = make_unique_wrapper(p9);
        std::unique_ptr w10 = make_unique_wrapper(p10);
        std::unique_ptr w11 = make_unique_wrapper(p11);
        std::unique_ptr w12 = make_unique_wrapper(p12);

        auto test = [&](const auto& parallelotope)
        {
                std::unique_ptr w = make_unique_wrapper(parallelotope);

                test_intersection(*w1, *w, false, "1-p");
                test_intersection(*w2, *w, true, "2-p");
                test_intersection(*w3, *w, false, "3-p");

                test_intersection(*w4, *w, false, "4-p");
                test_intersection(*w5, *w, true, "5-p");
                test_intersection(*w6, *w, false, "6-p");

                test_intersection(*w7, *w, false, "7-p");
                test_intersection(*w8, *w, false, "8-p");
                test_intersection(*w9, *w, false, "9-p");

                test_intersection(*w10, *w, false, "10-p");
                test_intersection(*w11, *w, true, "11-p");
                test_intersection(*w12, *w, false, "12-p");
        };

        constexpr std::array<T, N> EDGES = make_array_value<T, N>(SIZE);

        print_separator();
        LOG("ParallelotopeAA");

        test(ParallelotopeAA<N, T>(ORG, EDGES));

        print_separator();
        LOG("Parallelotope");

        test(Parallelotope<N, T>(ORG, to_edge_vector(EDGES)));

        print_separator();
        LOG("Check passed");
}

template <std::size_t N, typename T>
void all_tests(int point_count)
{
        test_points<N, T>(point_count);
        test_algorithms<N, T>();
        test_intersections<N, T>();
        test_intersections_hyperplane<N, T>();
}

void test_parallelotope_2()
{
        all_tests<2, double>(POINT_COUNT);
}
void test_parallelotope_3()
{
        all_tests<3, double>(POINT_COUNT);
}
void test_parallelotope_4()
{
        all_tests<4, double>(POINT_COUNT);
}

TEST_SMALL("2-Parallelotope", test_parallelotope_2)
TEST_SMALL("3-Parallelotope", test_parallelotope_3)
TEST_SMALL("4-Parallelotope", test_parallelotope_4)
}
}
