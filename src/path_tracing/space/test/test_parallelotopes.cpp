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
#include "path_tracing/space/parallelotope.h"
#include "path_tracing/space/parallelotope_ortho.h"

#include <algorithm>
#include <random>

constexpr double POSITION_DELTA = 1e-6;
constexpr double COMPARE_EPSILON = 1e-10;
constexpr double MAX_DOT_PRODUCT_OF_EDGES = 0.9;

using Parallelepiped = Parallelotope<3, double>;
using ParallelepipedOrtho = ParallelotopeOrtho<3, double>;

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

void three_edges(std::mt19937_64& engine, double min, double max, vec3* e0, vec3* e1, vec3* e2)
{
        std::uniform_real_distribution<double> urd_e(min, max);
        do
        {
                *e0 = vec3(urd_e(engine), urd_e(engine), urd_e(engine));
                *e1 = vec3(urd_e(engine), urd_e(engine), urd_e(engine));
                *e2 = vec3(urd_e(engine), urd_e(engine), urd_e(engine));
        } while (dot(normalize(*e0), normalize(*e1)) >= MAX_DOT_PRODUCT_OF_EDGES &&
                 dot(normalize(*e1), normalize(*e2)) >= MAX_DOT_PRODUCT_OF_EDGES &&
                 dot(normalize(*e0), normalize(*e2)) >= MAX_DOT_PRODUCT_OF_EDGES);
}

std::vector<vec3> outside_points(std::mt19937_64& engine, int cnt, vec3 org, vec3 e0, vec3 e1, vec3 e2)
{
        double e0_length = length(e0);
        double e1_length = length(e1);
        double e2_length = length(e2);

        vec3 p_e0 = normalize(e0);
        vec3 p_e1 = normalize(e1);
        vec3 p_e2 = normalize(e2);

        std::uniform_real_distribution<double> urd1_e0(e0_length * (1 + POSITION_DELTA), e0_length * 10);
        std::uniform_real_distribution<double> urd2_e0(-e0_length * 10, -POSITION_DELTA * e0_length);

        std::uniform_real_distribution<double> urd1_e1(e1_length * (1 + POSITION_DELTA), e1_length * 10);
        std::uniform_real_distribution<double> urd2_e1(-e1_length * 10, -POSITION_DELTA * e1_length);

        std::uniform_real_distribution<double> urd1_e2(e2_length * (1 + POSITION_DELTA), e2_length * 10);
        std::uniform_real_distribution<double> urd2_e2(-e2_length * 10, -POSITION_DELTA * e2_length);

        std::vector<vec3> res;

        for (int i = 0, j = 0; i < cnt; ++i)
        {
                double x = (++j & 1) ? urd1_e0(engine) : urd2_e0(engine);
                double y = (++j & 1) ? urd1_e1(engine) : urd2_e1(engine);
                double z = (++j & 1) ? urd1_e2(engine) : urd2_e2(engine);

                res.push_back(org + p_e0 * x + p_e1 * y + p_e2 * z);
        }

        return res;
}

std::vector<vec3> inside_points(std::mt19937_64& engine, int cnt, vec3 org, vec3 e0, vec3 e1, vec3 e2)
{
        double e0_length = length(e0);
        double e1_length = length(e1);
        double e2_length = length(e2);

        vec3 p_e0 = normalize(e0);
        vec3 p_e1 = normalize(e1);
        vec3 p_e2 = normalize(e2);

        std::uniform_real_distribution<double> urd_e0(e0_length * POSITION_DELTA, e0_length * (1 - POSITION_DELTA));
        std::uniform_real_distribution<double> urd_e1(e1_length * POSITION_DELTA, e1_length * (1 - POSITION_DELTA));
        std::uniform_real_distribution<double> urd_e2(e2_length * POSITION_DELTA, e2_length * (1 - POSITION_DELTA));

        std::vector<vec3> res;

        for (int i = 0; i < cnt; ++i)
        {
                res.push_back(org + p_e0 * urd_e0(engine) + p_e1 * urd_e1(engine) + p_e2 * urd_e2(engine));
        }

        return res;
}

std::vector<vec3> cover_points(std::mt19937_64& engine, int cnt, vec3 org, vec3 e0, vec3 e1, vec3 e2)
{
        double e0_length = length(e0);
        double e1_length = length(e1);
        double e2_length = length(e2);

        vec3 p_e0 = normalize(e0);
        vec3 p_e1 = normalize(e1);
        vec3 p_e2 = normalize(e2);

        std::uniform_real_distribution<double> urd_e0(-0.2 * e0_length, e0_length * 1.2);
        std::uniform_real_distribution<double> urd_e1(-0.2 * e1_length, e1_length * 1.2);
        std::uniform_real_distribution<double> urd_e2(-0.2 * e2_length, e2_length * 1.2);

        std::uniform_real_distribution<double> face_e0(0, e0_length);
        std::uniform_real_distribution<double> face_e1(0, e1_length);
        std::uniform_real_distribution<double> face_e2(0, e2_length);

        std::vector<vec3> res;

        for (int i = 0; i < cnt; ++i)
        {
                res.push_back(org + p_e0 * urd_e0(engine) + p_e1 * urd_e1(engine) + p_e2 * urd_e2(engine));

                res.push_back(org + p_e0 * face_e0(engine) + p_e1 * face_e1(engine));
                res.push_back(org + p_e0 * face_e0(engine) + p_e2 * face_e2(engine));
                res.push_back(org + p_e1 * face_e1(engine) + p_e2 * face_e2(engine));

                res.push_back(org + e2 + p_e0 * face_e0(engine) + p_e1 * face_e1(engine));
                res.push_back(org + e1 + p_e0 * face_e0(engine) + p_e2 * face_e2(engine));
                res.push_back(org + e0 + p_e1 * face_e1(engine) + p_e2 * face_e2(engine));
        }

        return res;
}

template <typename P>
void test_parallelepiped(int point_count, const P& d)
{
        double max_length = max_diagonal(d);

        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

        for (vec3 v : outside_points(engine, point_count, d.org(), d.e(0), d.e(1), d.e(2)))
        {
                if (d.inside(v))
                {
                        error("point must be outside\n" + to_string(v));
                }
        }

        std::uniform_real_distribution<double> urd_dir(-1, 1);

        for (vec3 origin : inside_points(engine, point_count, d.org(), d.e(0), d.e(1), d.e(2)))
        {
                if (!d.inside(origin))
                {
                        error("point must be inside\n" + to_string(origin));
                }

                vec3 direction;
                do
                {
                        direction = vec3(urd_dir(engine), urd_dir(engine), urd_dir(engine));
                } while (length(direction) == 0);

                ray3 ray_orig(origin, direction);

                double t;
                ray3 ray;

                ray = ray_orig;

                if (!d.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }
                if (t >= max_length)
                {
                        error("intersection out of parallelepiped.\ndistance = " + to_string(t) + ", " +
                              "max distance = " + to_string(max_length) + "\n" + to_string(ray));
                }

                ray = ray3(ray_orig.point(-10 * max_length), direction);
                if (!d.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }

                ray = ray3(ray_orig.point(10 * max_length), -direction);
                if (!d.intersect(ray, &t))
                {
                        error("ray must intersect\n" + to_string(ray));
                }

                ray = ray3(ray_orig.point(10 * max_length), direction);
                if (d.intersect(ray, &t))
                {
                        error("ray must not intersect\n" + to_string(ray));
                }

                ray = ray3(ray_orig.point(-10 * max_length), -direction);
                if (d.intersect(ray, &t))
                {
                        error("ray must not intersect\n" + to_string(ray));
                }
        }
}

template <typename... P>
void verify_intersection(const ray3& ray, const P&... d)
{
        std::array<double, sizeof...(P)> ts;
        unsigned i = 0;
        std::array<bool, sizeof...(P)> intersections{{d.intersect(ray, &ts[i++])...}};

        for (i = 1; i < sizeof...(P); ++i)
        {
                if (intersections[i] != intersections[0])
                {
                        error("Error intersect\n" + to_string(ray));
                }
                if (intersections[i] && std::abs(ts[i] - ts[0]) > COMPARE_EPSILON)
                {
                        error("Error intersection distance.\nDistance = " + to_string(ts[i]) +
                              ", first distance  = " + to_string(ts[0]) + "\n" + to_string(ray));
                }
        }
}

template <typename... P>
void verify_vectors(const std::array<vec3, sizeof...(P)>& vectors, const std::string& name)
{
        for (unsigned i = 1; i < sizeof...(P); ++i)
        {
                if (length(vectors[i] - vectors[0]) > COMPARE_EPSILON)
                {
                        error("Error " + name);
                }
        }
}

template <typename... P>
void compare_parallelepipeds(int point_count, const P&... d)
{
        std::array<double, sizeof...(P)> max_length{{max_diagonal(d)...}};
        for (unsigned i = 1; i < sizeof...(P); ++i)
        {
                if (std::abs(max_length[i] - max_length[0]) > COMPARE_EPSILON)
                {
                        error("Error max length");
                }
        }

        std::array<vec3, sizeof...(P)> orgs{{d.org()...}};
        verify_vectors<P...>(orgs, "orgs");

        std::array<vec3, sizeof...(P)> e0s{{d.e(0)...}};
        verify_vectors<P...>(e0s, "e0s");

        std::array<vec3, sizeof...(P)> e1s{{d.e(1)...}};
        verify_vectors<P...>(e1s, "e1s");

        std::array<vec3, sizeof...(P)> e2s{{d.e(2)...}};
        verify_vectors<P...>(e2s, "e2s");

        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

        std::uniform_real_distribution<double> urd_dir(-1, 1);
        std::uniform_int_distribution<int> uid_dir(-1, 1);
        std::uniform_int_distribution<int> uid_select(0, 10);

        for (vec3 origin : cover_points(engine, point_count, orgs[0], e0s[0], e1s[0], e2s[0]))
        {
                std::array<bool, sizeof...(P)> inside{{d.inside(origin)...}};
                for (unsigned i = 1; i < sizeof...(P); ++i)
                {
                        if (inside[i] != inside[0])
                        {
                                error("Error point inside\n" + to_string(origin));
                        }
                }

                vec3 direction;
                do
                {
                        direction = vec3(uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine),
                                         uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine),
                                         uid_select(engine) != 0 ? urd_dir(engine) : uid_dir(engine));
                } while (length(direction) == 0);

                ray3 ray_orig(origin, direction);

                ray3 ray;

                ray = ray3(ray_orig.get_org(), direction);

                verify_intersection(ray, d...);

                ray = ray3(ray_orig.point(-10 * max_length[0]), direction);

                verify_intersection(ray, d...);

                ray = ray3(ray_orig.point(10 * max_length[0]), -direction);

                verify_intersection(ray, d...);

                ray = ray3(ray_orig.point(10 * max_length[0]), direction);

                verify_intersection(ray, d...);

                ray = ray3(ray_orig.point(-10 * max_length[0]), -direction);

                verify_intersection(ray, d...);
        }
}
}

void test_parallelotopes()
{
        constexpr int point_count = 1000000;

        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());
        std::uniform_real_distribution<double> urd_org(-10, 10);

        vec3 org = vec3(urd_org(engine), urd_org(engine), urd_org(engine));

        {
                std::uniform_real_distribution<double> urd_e(0.1, 20);
                vec3 e(urd_e(engine), urd_e(engine), urd_e(engine));
                ParallelepipedOrtho d(org, e[0], e[1], e[2]);

                LOG("test parallelepiped ortho, org " + to_string(org) + ", e " + to_string(e));

                test_parallelepiped(point_count, d);
        }

        {
                vec3 e0, e1, e2;
                three_edges(engine, -20, 20, &e0, &e1, &e2);
                Parallelepiped d(org, e0, e1, e2);

                LOG("test parallelepiped, org = " + to_string(org) + ", e0 = " + to_string(e0) + ", e1 = " + to_string(e1) +
                    ", e2 = " + to_string(e2));

                test_parallelepiped(point_count, d);
        }

        {
                std::uniform_real_distribution<double> urd_e(0.1, 20);
                vec3 e(urd_e(engine), urd_e(engine), urd_e(engine));

                Parallelepiped d(org, vec3(e[0], 0, 0), vec3(0, e[1], 0), vec3(0, 0, e[2]));
                ParallelepipedOrtho d_ortho(org, e[0], e[1], e[2]);

                LOG("compare parallelepipeds, org = " + to_string(org) + ", e0 = " + to_string(e[0]) +
                    ", e1 = " + to_string(e[1]) + ", e2 = " + to_string(e[2]));

                compare_parallelepipeds(point_count, d, d_ortho);
        }

        LOG("test parallelepiped done");
}
