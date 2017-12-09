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

#include "test_parallelotopes.h"

#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/random.h"
#include "com/ray.h"
#include "com/vec.h"
#include "path_tracing/random/random_vector.h"
#include "path_tracing/space/parallelotope.h"
#include "path_tracing/space/parallelotope_ortho.h"

#include <algorithm>
#include <random>
#include <utility>

template <typename T>
constexpr T POSITION_DELTA = 1e-6;

template <typename T>
constexpr T COMPARE_EPSILON = 1e-10;

template <typename T>
constexpr T MAX_DOT_PRODUCT_OF_EDGES = 0.9;

template <typename Parallelotope>
using VectorP = Vector<Parallelotope::DIMENSION, typename Parallelotope::DataType>;

namespace
{
template <typename Parallelotope, typename F>
void all_diagonals(const Parallelotope& p, const VectorP<Parallelotope>& edge_sum, int n, const F& f)
{
        if (n >= 0)
        {
                all_diagonals(p, edge_sum + p.e(n), n - 1, f);
                all_diagonals(p, edge_sum - p.e(n), n - 1, f);
        }
        else
        {
                f(edge_sum);
        }
}
template <typename Parallelotope, typename F>
void all_diagonals(const Parallelotope& p, const F& f)
{
        constexpr int last_index = Parallelotope::DIMENSION - 1;
        // Одно из измерений не меняется, остальные к нему прибавляются и вычитаются
        all_diagonals(p, p.e(last_index), last_index - 1, f);
}
template <typename Parallelotope>
typename Parallelotope::DataType max_diagonal(const Parallelotope& parallelotope)
{
        using T = typename Parallelotope::DataType;

        // Перебрать все диагонали одной из граней параллелотопа с учётом их направления.
        // Количество таких диагоналей равно 2 ^ (N - 1). Добавляя к каждой такой
        // диагонали оставшееся измерение, получаются все диагонали целого параллелотопа.
        // Надо найти максимум из их длин.

        T max_length = std::numeric_limits<T>::lowest();

        all_diagonals(parallelotope,
                      [&max_length](const VectorP<Parallelotope>& d) { max_length = std::max(max_length, length(d)); });

        return max_length;
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

template <typename RandomEngine, typename Parallelotope>
void test_parallelotope(RandomEngine& engine, int point_count, const Parallelotope& p)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        T max_length = max_diagonal(p);

        for (const Vector<N, T>& v : external_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
        {
                if (p.inside(v))
                {
                        error("point must be outside\n" + to_string(v));
                }
        }

        std::uniform_real_distribution<T> urd_dir(-1, 1);

        for (const Vector<N, T>& origin : internal_points(engine, point_count, p, std::make_integer_sequence<size_t, N>()))
        {
                if (!p.inside(origin))
                {
                        error("point must be inside\n" + to_string(origin));
                }

                Vector<N, T> direction;
                do
                {
                        direction = random_vector<N, T>(engine, urd_dir);
                } while (length(direction) < COMPARE_EPSILON<T>);

                Ray<N, T> ray_orig(origin, direction);

                T t;
                Ray<N, T> ray;

                ray = ray_orig;

                if (!p.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }
                if (t >= max_length)
                {
                        error("intersection out of parallelotope.\ndistance = " + to_string(t) + ", " +
                              "max distance = " + to_string(max_length) + "\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * max_length), direction);
                if (!p.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * max_length), -direction);
                if (!p.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(10 * max_length), direction);
                if (p.intersect(ray, &t))
                {
                        error("ray must not intersect\n" + to_string(ray));
                }

                ray = Ray<N, T>(ray_orig.point(-10 * max_length), -direction);
                if (p.intersect(ray, &t))
                {
                        error("ray must not intersect\n" + to_string(ray));
                }
        }
}

template <size_t N, typename T, typename... Parallelotope>
void verify_intersection(const Ray<N, T>& ray, const Parallelotope&... p)
{
        static_assert(((N == Parallelotope::DIMENSION) && ...));
        static_assert(((std::is_same_v<T, typename Parallelotope::DataType>)&&...));

        std::array<T, sizeof...(Parallelotope)> ts;
        unsigned i = 0;
        std::array<bool, sizeof...(Parallelotope)> intersections{{p.intersect(ray, &ts[i++])...}};

        for (i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (intersections[i] != intersections[0])
                {
                        error("Error intersect\n" + to_string(ray));
                }
                if (intersections[i] && std::abs(ts[i] - ts[0]) > COMPARE_EPSILON<T>)
                {
                        error("Error intersection distance.\nDistance = " + to_string(ts[i]) +
                              ", first distance  = " + to_string(ts[0]) + "\n" + to_string(ray));
                }
        }
}

template <size_t N, typename T, size_t Count>
void verify_vectors(const std::array<Vector<N, T>, Count>& vectors, const std::string& name)
{
        for (unsigned i = 1; i < Count; ++i)
        {
                if (length(vectors[i] - vectors[0]) > COMPARE_EPSILON<T>)
                {
                        error("Error " + name);
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

        std::array<T, sizeof...(Parallelotope)> max_length{{max_diagonal(p)...}};

        for (unsigned i = 1; i < sizeof...(Parallelotope); ++i)
        {
                if (std::abs(max_length[i] - max_length[0]) > COMPARE_EPSILON<T>)
                {
                        error("Error max length");
                }
        }

        std::array<Vector<N, T>, sizeof...(Parallelotope)> orgs{{p.org()...}};
        verify_vectors(orgs, "orgs");

        for (unsigned i = 0; i < N; ++i)
        {
                std::array<Vector<N, T>, sizeof...(Parallelotope)> e{{p.e(i)...}};
                verify_vectors(e, "e" + to_string(i));
        }

        std::uniform_real_distribution<T> urd_dir(-1, 1);
        std::uniform_int_distribution<int> uid_dir(-1, 1);
        std::uniform_int_distribution<int> uid_select(0, 10);

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

                Vector<N, T> direction;
                do
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                direction[i] = uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine);
                        }
                } while (length(direction) < COMPARE_EPSILON<T>);

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
void test_parallelotopes()
{
        constexpr int point_count = 100000;

        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

        std::uniform_real_distribution<T> urd_org(-10, 10);

        Vector<N, T> org = random_vector<N, T>(engine, urd_org);

        {
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_ortho_edges<N, T>(engine, urd);
                ParallelotopeOrtho<N, T> p_ortho(org, edges);

                LOG("---\ntest parallelotope ortho\n" + to_string(p_ortho));

                test_parallelotope(engine, point_count, p_ortho);
        }

        {
                std::uniform_real_distribution<T> urd(-20.0, 20.0);
                std::array<Vector<N, T>, N> edges = random_edges<N, T>(engine, urd);
                Parallelotope<N, T> p(org, edges);

                LOG("---\ntest parallelotope\n" + to_string(p));

                test_parallelotope(engine, point_count, p);
        }

        {
                std::uniform_real_distribution<T> urd(0.1, 20);
                std::array<T, N> edges = random_ortho_edges<N, T>(engine, urd);

                ParallelotopeOrtho<N, T> p_ortho(org, edges);
                Parallelotope<N, T> p(org, to_edge_vector(edges));

                LOG("---\ntest parallelotope comparison\n#1\n" + to_string(p_ortho) + "\n#2\n" + to_string(p));

                compare_parallelotopes(engine, point_count, p_ortho, p);
        }

        LOG("---\ntest parallelotope done");
}
}

void test_parallelotopes()
{
        test_parallelotopes<2, double>();
        test_parallelotopes<3, double>();
        test_parallelotopes<4, double>();
        test_parallelotopes<5, double>();
}
