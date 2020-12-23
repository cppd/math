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

#include "test_parallelotope.h"

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
#include <src/numerical/random.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/utility/random/engine.h>

#include <algorithm>
#include <memory>
#include <random>
#include <utility>

namespace painter
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

template <size_t N, typename T>
bool almost_equal(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return (a - b).norm() <= EQUALITY_EPSILON<T>;
}

template <size_t N, typename T>
bool test_edge_angles(const std::array<Vector<N, T>, N>& unit_edges)
{
        for (unsigned i = 0; i < N; ++i)
        {
                for (unsigned j = i + 1; j < N; ++j)
                {
                        if (std::abs(dot(unit_edges[i], unit_edges[j])) >= MAX_DOT_PRODUCT_OF_EDGES<T>)
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <size_t N, typename T, typename RandomEngine, typename Distribution>
std::array<Vector<N, T>, N> random_edges(RandomEngine& engine, Distribution& distribution)
{
        std::array<Vector<N, T>, N> edges;

        do
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        edges[i] = random_vector<N, T>(engine, distribution).normalized();
                }

        } while (!test_edge_angles(edges));

        return edges;
}

template <size_t N, typename T, typename RandomEngine, typename Distribution>
std::array<T, N> random_aa_edges(RandomEngine& engine, Distribution& distribution)
{
        std::array<T, N> edges;
        for (unsigned i = 0; i < N; ++i)
        {
                edges[i] = distribution(engine);
        }
        return edges;
}

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> external_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::SPACE_DIMENSION;
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

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> internal_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::SPACE_DIMENSION;
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

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> cover_points(
        RandomEngine& engine,
        int count,
        const Parallelotope& p,
        std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::SPACE_DIMENSION;
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

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_direction(RandomEngine& engine)
{
        std::uniform_real_distribution<T> urd_dir(-1, 1);

        // Равновероятность всех направлений не нужна
        while (true)
        {
                Vector<N, T> direction = random_vector<N, T>(engine, urd_dir);

                if (direction.norm_squared() > 0)
                {
                        return direction;
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
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

                if (direction.norm_squared() > 0)
                {
                        return direction;
                }
        }
}

template <size_t N, size_t Count, typename T>
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
        constexpr size_t N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        T length = p.length();

        const Constraints<N, T, 2 * N, 0> constraints = p.constraints();

        for (const Vector<N, T>& point :
             external_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
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
             internal_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
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

template <size_t N, typename T, typename... Parallelotope>
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

template <size_t N, typename T, size_t Count>
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

        constexpr size_t N = std::get<0>(std::make_tuple(Parallelotope::SPACE_DIMENSION...));
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
                     engine, point_count, std::get<0>(std::make_tuple(p...)), std::make_integer_sequence<size_t, N>()))
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

template <size_t N, typename T>
std::array<Vector<N, T>, N> to_edge_vector(const std::array<T, N>& edges)
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

template <size_t N, typename T>
std::array<Vector<N + 1, T>, N> to_edge_vector_hyper(const std::array<T, N>& edges)
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

template <size_t N, typename T>
void test_points(int point_count)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        std::uniform_real_distribution<T> urd_org(-10, 10);

        LOG("------------------------------");
        LOG("Parallelotope points in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_aa_edges<N, T>(engine, urd);
                ParallelotopeAA<N, T> p(org, edges);

                print_message(to_string(p));

                test_points(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(-20.0, 20.0);
                std::array<Vector<N, T>, N> edges = random_edges<N, T>(engine, urd);
                Parallelotope<N, T> p(org, edges);

                print_message(to_string(p));

                test_points(engine, point_count, p);
        }

        print_separator();
        LOG("Parallelotope comparison");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_aa_edges<N, T>(engine, urd);

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

        print_separator();
        print_message("vertex ridges");
        for (auto vr : p.vertex_ridges())
        {
                print_message(to_string(vr));
        }
}

template <size_t N, typename T>
void test_algorithms()
{
        constexpr std::array<T, N> edges = make_array_value<T, N>(1);
        constexpr Vector<N, T> org(0);

        LOG("------------------------------");
        LOG("Parallelotope algorithms in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p(org, edges);
                test_algorithms(p);
        }

        print_separator();
        LOG("Parallelotope");

        {
                Parallelotope<N, T> p(org, to_edge_vector(edges));
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

template <size_t N, typename T>
void test_intersections()
{
        std::array<T, N> edges = make_array_value<T, N>(1);
        Vector<N, T> org0(0.0);
        Vector<N, T> org1(0.75);
        Vector<N, T> org2(1.5);

        Vector<N, T> org_big(-5);
        std::array<T, N> edges_big = make_array_value<T, N>(10);

        LOG("------------------------------");
        LOG("Parallelotope intersections in " + space_name(N));

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p1(org0, edges);
                ParallelotopeAA<N, T> p2(org1, edges);
                ParallelotopeAA<N, T> p3(org2, edges);
                ParallelotopeAA<N, T> p_big(org_big, edges_big);

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
                Parallelotope<N, T> p1(org0, to_edge_vector(edges));
                Parallelotope<N, T> p2(org1, to_edge_vector(edges));
                Parallelotope<N, T> p3(org2, to_edge_vector(edges));
                Parallelotope<N, T> p_big(org_big, to_edge_vector(edges_big));

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

template <size_t N, typename T>
void test_intersections_hyperplane()
{
        Vector<N, T> org(5);
        std::array<T, N> edges = make_array_value<T, N>(1);

        std::array<Vector<N, T>, N - 1> edges_hyper_big = to_edge_vector_hyper(make_array_value<T, N - 1>(3));
        Vector<N, T> org1(4);
        Vector<N, T> org2(4);
        Vector<N, T> org3(4);
        org1[N - 1] = 4.9;
        org2[N - 1] = 5.5;
        org3[N - 1] = 6.1;

        std::array<Vector<N, T>, N - 1> edges_hyper_small = to_edge_vector_hyper(make_array_value<T, N - 1>(0.2));
        Vector<N, T> org4(4.9);
        Vector<N, T> org5(4.9);
        Vector<N, T> org6(4.9);
        org4[N - 1] = 4.9;
        org5[N - 1] = 5.5;
        org6[N - 1] = 6.1;
        Vector<N, T> org7(4);
        Vector<N, T> org8(4);
        Vector<N, T> org9(4);
        org7[N - 1] = 4.9;
        org8[N - 1] = 5.5;
        org9[N - 1] = 6.1;
        Vector<N, T> org10(5.5);
        Vector<N, T> org11(5.5);
        Vector<N, T> org12(5.5);
        org10[N - 1] = 4.9;
        org11[N - 1] = 5.5;
        org12[N - 1] = 6.1;

        LOG("------------------------------");
        LOG("Hyperplane parallelotope intersections in " + space_name(N));

        HyperplaneParallelotope<N, T> p1(org1, edges_hyper_big);
        HyperplaneParallelotope<N, T> p2(org2, edges_hyper_big);
        HyperplaneParallelotope<N, T> p3(org3, edges_hyper_big);
        HyperplaneParallelotope<N, T> p4(org4, edges_hyper_small);
        HyperplaneParallelotope<N, T> p5(org5, edges_hyper_small);
        HyperplaneParallelotope<N, T> p6(org6, edges_hyper_small);
        HyperplaneParallelotope<N, T> p7(org7, edges_hyper_small);
        HyperplaneParallelotope<N, T> p8(org8, edges_hyper_small);
        HyperplaneParallelotope<N, T> p9(org9, edges_hyper_small);
        HyperplaneParallelotope<N, T> p10(org10, edges_hyper_small);
        HyperplaneParallelotope<N, T> p11(org11, edges_hyper_small);
        HyperplaneParallelotope<N, T> p12(org12, edges_hyper_small);

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

        print_separator();
        LOG("ParallelotopeAA");

        {
                ParallelotopeAA<N, T> p(org, edges);
                std::unique_ptr w = make_unique_wrapper(p);

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
        }

        print_separator();
        LOG("Parallelotope");

        {
                Parallelotope<N, T> p(org, to_edge_vector(edges));
                std::unique_ptr w = make_unique_wrapper(p);

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
        }

        print_separator();
        LOG("Check passed");
}

template <size_t N, typename T>
void all_tests(int point_count)
{
        test_points<N, T>(point_count);
        test_algorithms<N, T>();
        test_intersections<N, T>();
        test_intersections_hyperplane<N, T>();
}
}

void test_parallelotope(int number_of_dimensions)
{
        switch (number_of_dimensions)
        {
        case 2:
                all_tests<2, double>(POINT_COUNT);
                break;
        case 3:
                all_tests<3, double>(POINT_COUNT);
                break;
        case 4:
                all_tests<4, double>(POINT_COUNT);
                break;
        default:
                error("Error parallelotope test number of dimensions " + to_string(number_of_dimensions));
        }
}
}
