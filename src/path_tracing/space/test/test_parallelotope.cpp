/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/arrays.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random/engine.h"
#include "com/random/vector.h"
#include "com/ray.h"
#include "com/vec.h"
#include "path_tracing/constants.h"
#include "path_tracing/space/parallelotope.h"
#include "path_tracing/space/parallelotope_algorithm.h"
#include "path_tracing/space/parallelotope_ortho.h"
#include "path_tracing/space/parallelotope_wrapper.h"
#include "path_tracing/space/shape_intersection.h"

#include <algorithm>
#include <memory>
#include <random>
#include <utility>

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

template <typename T>
constexpr T COMPARISON_DIRECTION_EPSILON;
template <>
constexpr double COMPARISON_DIRECTION_EPSILON<double> = 2 * EPSILON<double>;

template <typename Parallelotope>
using VectorP = Vector<Parallelotope::DIMENSION, typename Parallelotope::DataType>;

namespace
{
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
        return length(a - b) <= EQUALITY_EPSILON<T>;
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
                        edges[i] = normalize(random_vector<N, T>(engine, distribution));
                }

        } while (!test_edge_angles(edges));

        return edges;
}

template <size_t N, typename T, typename RandomEngine, typename Distribution>
std::array<T, N> random_ortho_edges(RandomEngine& engine, Distribution& distribution)
{
        std::array<T, N> edges;
        for (unsigned i = 0; i < N; ++i)
        {
                edges[i] = distribution(engine);
        }
        return edges;
}

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> external_points(RandomEngine& engine, int count, const Parallelotope& p,
                                                    std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {{length(p.e(I))...}};

        std::array<std::uniform_real_distribution<T>, N> low = {
                {std::uniform_real_distribution<T>{-len[I] * 10, -POSITION_DELTA<T> * len[I]}...}};
        std::array<std::uniform_real_distribution<T>, N> high = {
                {std::uniform_real_distribution<T>{len[I] * (1 + POSITION_DELTA<T>), len[I] * 10}...}};

        std::uniform_int_distribution<int> rnd(0, 1);

        std::array<Vector<N, T>, N> unit = {{(p.e(I) / len[I])...}};

        std::vector<Vector<N, T>> points;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((rnd(engine) ? low[I](engine) : high[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> internal_points(RandomEngine& engine, int count, const Parallelotope& p,
                                                    std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {{length(p.e(I))...}};

        std::array<std::uniform_real_distribution<T>, N> internal = {
                {std::uniform_real_distribution<T>{len[I] * POSITION_DELTA<T>, len[I] * (1 - POSITION_DELTA<T>)}...}};

        std::array<Vector<N, T>, N> unit = {{(p.e(I) / len[I])...}};

        std::vector<Vector<N, T>> points;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v((internal[I](engine))...);

                points.push_back(p.org() + ((unit[I] * v[I]) + ...));
        }

        return points;
}

template <typename Parallelotope, typename RandomEngine, size_t... I>
std::vector<VectorP<Parallelotope>> cover_points(RandomEngine& engine, int count, const Parallelotope& p,
                                                 std::integer_sequence<size_t, I...>)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(sizeof...(I) == N);

        std::array<T, N> len = {{length(p.e(I))...}};

        std::array<std::uniform_real_distribution<T>, N> cover = {
                {std::uniform_real_distribution<T>{static_cast<T>(-0.2) * len[I], len[I] * static_cast<T>(1.2)}...}};

        std::array<std::uniform_real_distribution<T>, N> len_random = {{std::uniform_real_distribution<T>{0, len[I]}...}};

        std::array<Vector<N, T>, N> unit = {{(p.e(I) / len[I])...}};

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
                if (length(direction) > 0)
                {
                        return direction;
                }
        }
}

template <typename RandomEngine, typename Parallelotope>
void test_points(RandomEngine& engine, int point_count, const Parallelotope& p)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        T max_length = parallelotope_max_diagonal(p);

        for (const Vector<N, T>& point : external_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
        {
                if (p.inside(point))
                {
                        error("Point must be outside\n" + to_string(point));
                }
        }

        for (const Vector<N, T>& origin : internal_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
        {
                if (!p.inside(origin))
                {
                        error("Point must be inside\n" + to_string(origin));
                }

                Vector<N, T> direction = random_direction<N, T>(engine);

                Ray<N, T> ray_orig(origin, direction);

                T t;
                Ray<N, T> ray;

                ray = ray_orig;

                if (!p.intersect(ray, &t))
                {
                        error("Ray must intersect\n" + to_string(ray));
                }
                if (t >= max_length)
                {
                        error("Intersection out of parallelotope.\ndistance = " + to_string(t) + ", " +
                              "max distance = " + to_string(max_length) + "\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * max_length), direction);
                if (!p.intersect(ray, &t))
                {
                        error("Ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * max_length), -direction);
                if (!p.intersect(ray, &t))
                {
                        error("Ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * max_length), direction);
                if (p.intersect(ray, &t))
                {
                        error("Ray must not intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * max_length), -direction);
                if (p.intersect(ray, &t))
                {
                        error("Ray must not intersect\n" + to_string(ray));
                }
        }
}

template <size_t N, typename T, typename... Parallelotope>
void verify_intersection(const Ray<N, T>& ray, const Parallelotope&... p)
{
        static_assert(((N == Parallelotope::DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<T, sizeof...(Parallelotope)> distances;
        unsigned i = 0;
        std::array<bool, sizeof...(Parallelotope)> intersections{{p.intersect(ray, &distances[i++])...}};

        for (i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (intersections[i] != intersections[0])
                {
                        error("Error intersection comparison\n" + to_string(ray));
                }
                if (intersections[i] && !almost_equal(distances[i], distances[0]))
                {
                        error("Error intersection distance comparison.\nDistance = " + to_string(distances[i]) +
                              ", first distance  = " + to_string(distances[0]) + "\n" + to_string(ray));
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
                        error("Error comparison of " + name + ".\n" + to_string(vectors[i]) + " and " + to_string(vectors[0]));
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
                T direction_length;

                // Равновероятность всех направлений не нужна
                do
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                direction[i] = uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine);
                        }

                        direction_length = length(direction);

                } while (direction_length == 0);

                // Разные типы параллелотопов могут по-разному обрабатывать лучи,
                // почти параллельные плоскостям, поэтому надо исключить такие лучи.
                Vector<N, T> unit_direction = direction / direction_length;
                bool use_direction = true;
                for (unsigned i = 0; i < N; ++i)
                {
                        if (std::abs(unit_direction[i]) > 0 && std::abs(unit_direction[i]) <= COMPARISON_DIRECTION_EPSILON<T>)
                        {
                                use_direction = false;
                                break;
                        }
                }
                if (use_direction)
                {
                        return direction;
                }
        }
}

template <typename RandomEngine, typename... Parallelotope>
void compare_parallelotopes(RandomEngine& engine, int point_count, const Parallelotope&... p)
{
        static_assert(sizeof...(Parallelotope) >= 2);

        constexpr size_t N = std::get<0>(std::make_tuple(Parallelotope::DIMENSION...));
        using T = typename std::tuple_element_t<0, std::tuple<Parallelotope...>>::DataType;

        static_assert(((N == Parallelotope::DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<T, sizeof...(Parallelotope)> max_length{{parallelotope_max_diagonal(p)...}};

        for (unsigned i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (!almost_equal(max_length[i], max_length[0]))
                {
                        error("Error diagonal max length.\n" + to_string(max_length[i]) + " and " + to_string(max_length[0]));
                }
        }

        std::array<Vector<N, T>, sizeof...(Parallelotope)> orgs{{p.org()...}};
        verify_vectors(orgs, "orgs");

        for (unsigned i = 0; i < N; ++i)
        {
                std::array<Vector<N, T>, sizeof...(Parallelotope)> e{{p.e(i)...}};
                verify_vectors(e, "e" + to_string(i));
        }

        for (Vector<N, T> origin :
             cover_points(engine, point_count, std::get<0>(std::make_tuple(p...)), std::make_integer_sequence<size_t, N>()))
        {
                std::array<bool, sizeof...(Parallelotope)> inside{{p.inside(origin)...}};
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

                ray = Ray<N, T>(ray_orig.get_org(), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(-10 * max_length[0]), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(10 * max_length[0]), -direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(10 * max_length[0]), direction);

                verify_intersection(ray, p...);

                ray = Ray<N, T>(ray_orig.point(-10 * max_length[0]), -direction);

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
void test_points(int point_count)
{
        RandomEngineWithSeed<std::mt19937_64> engine;

        std::uniform_real_distribution<T> urd_org(-10, 10);

        LOG("------------------------------");
        LOG("Parallelotope points in " + to_string(N) + "D");

        print_separator();
        LOG("parallelotope ortho");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_ortho_edges<N, T>(engine, urd);
                ParallelotopeOrtho<N, T> p_ortho(org, edges);

                print_message(to_string(p_ortho));

                test_points(engine, point_count, p_ortho);
        }

        print_separator();
        LOG("parallelotope");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(-20.0, 20.0);
                std::array<Vector<N, T>, N> edges = random_edges<N, T>(engine, urd);
                Parallelotope<N, T> p(org, edges);

                print_message(to_string(p));

                test_points(engine, point_count, p);
        }

        print_separator();
        LOG("parallelotope comparison");

        {
                Vector<N, T> org = random_vector<N, T>(engine, urd_org);
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_ortho_edges<N, T>(engine, urd);

                ParallelotopeOrtho<N, T> p_ortho(org, edges);
                Parallelotope<N, T> p(org, to_edge_vector(edges));

                print_message("#1\n" + to_string(p_ortho) + "\n#2\n" + to_string(p));

                compare_parallelotopes(engine, point_count, p_ortho, p);
        }

        print_separator();
        LOG("check passed");
}

template <typename Parallelotope>
void test_algorithms(const Parallelotope& p)
{
        print_separator();
        print_message("diagonals");
        for (auto d : parallelotope_diagonals(p))
        {
                print_message(to_string(d));
        }

        print_separator();
        print_message("vertices");
        for (auto v : parallelotope_vertices(p))
        {
                print_message(to_string(v));
        }

        print_separator();
        print_message("vertex ridges");
        for (auto vr : parallelotope_vertex_ridges(p))
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
        LOG("Parallelotope algorithms in " + to_string(N) + "D");

        print_separator();
        LOG("parallelotope ortho");

        {
                ParallelotopeOrtho<N, T> p(org, edges);
                test_algorithms(p);
        }

        print_separator();
        LOG("parallelotope");

        {
                Parallelotope<N, T> p(org, to_edge_vector(edges));
                test_algorithms(p);
        }

        print_separator();
        LOG("check passed");
}

template <typename Parallelotope1, typename Parallelotope2>
void test_intersection(const Parallelotope1& p1, const Parallelotope2& p2, bool with_intersection, const std::string& text)
{
        if (with_intersection != shape_intersection(p1, p2))
        {
                error("Error intersection " + text);
        }

        print_message("intersection " + text);
}

template <typename Parallelotope>
std::unique_ptr<ParallelotopeWrapperForShapeIntersection<Parallelotope>> make_unique_wrapper(const Parallelotope& p)
{
        return std::make_unique<ParallelotopeWrapperForShapeIntersection<Parallelotope>>(p);
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
        LOG("Parallelotope intersections in " + to_string(N) + "D");

        print_separator();
        LOG("parallelotope ortho");

        {
                ParallelotopeOrtho<N, T> p1(org0, edges);
                ParallelotopeOrtho<N, T> p2(org1, edges);
                ParallelotopeOrtho<N, T> p3(org2, edges);
                ParallelotopeOrtho<N, T> p_big(org_big, edges_big);

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
        LOG("parallelotope");

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
        LOG("check passed");
}

template <size_t N, typename T>
void all_tests(int point_count)
{
        test_points<N, T>(point_count);
        test_algorithms<N, T>();
        test_intersections<N, T>();
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
