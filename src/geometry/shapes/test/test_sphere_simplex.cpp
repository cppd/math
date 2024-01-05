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

#include "compare.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_simplex.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>
#include <string>

namespace ns::geometry::shapes::test
{
namespace
{
template <std::size_t N, typename T>
void test_integrate(progress::Ratio* const progress, const double progress_min, const double progress_max)
{
        static_assert(std::is_floating_point_v<T>);

        LOG(std::string("Test sphere " + to_string(N - 1) + "-simplex, integrate, ") + type_name<T>());

        constexpr unsigned RAY_COUNT = 10'000'000;
        constexpr T RELATIVE_PRECISION = 0.01;
        constexpr T MIN_RELATIVE_AREA = 0.01;

        PCG engine;

        const std::array<Vector<N, T>, N> simplex_vertices = [&]()
        {
                std::array<Vector<N, T>, N> res;
                for (Vector<N, T>& v : res)
                {
                        v = sampling::uniform_on_sphere<N, T>(engine);
                }
                return res;
        }();

        const T sphere_area = SPHERE_AREA<N, T>;
        const T simplex_area = sphere_simplex_area(simplex_vertices);
        const T relative_area = simplex_area / sphere_area;

        if (!std::isfinite(relative_area))
        {
                error("Relative area " + to_string(relative_area) + " is not finite, sphere area = "
                      + to_string(sphere_area) + ", simplex area = " + to_string(simplex_area));
        }
        if (!(relative_area > 0))
        {
                error("Relative area " + to_string(relative_area) + " is not positive, sphere area = "
                      + to_string(sphere_area) + ", simplex area = " + to_string(simplex_area));
        }

        const spatial::HyperplaneSimplex<N, T> simplex(simplex_vertices);

        constexpr double RAY_COUNT_R = 1.0 / RAY_COUNT;
        unsigned intersect_count = 0;
        progress->set(progress_min);
        for (unsigned i = 0; i < RAY_COUNT; ++i)
        {
                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(std::lerp(progress_min, progress_max, i * RAY_COUNT_R));
                }
                const Ray<N, T> ray(Vector<N, T>(0), sampling::uniform_on_sphere<N, T>(engine));
                if (simplex.intersect(ray))
                {
                        ++intersect_count;
                }
        }

        const T coverage_area = static_cast<T>(intersect_count) / RAY_COUNT;

        if (coverage_area < MIN_RELATIVE_AREA && relative_area < MIN_RELATIVE_AREA)
        {
                return;
        }

        const T relative_error = std::abs(relative_area - coverage_area) / std::max(relative_area, coverage_area);
        if (relative_error < RELATIVE_PRECISION)
        {
                return;
        }

        std::ostringstream oss;
        oss << "Sphere area = " << sphere_area << '\n';
        oss << "Simplex area = " << simplex_area << '\n';
        oss << "Relative area = " << relative_area << '\n';
        oss << "Coverage area = " << coverage_area << '\n';
        oss << "Relative error = " << relative_error;
        error(oss.str());
}

template <std::size_t N, typename T, typename RandomEngine>
std::array<Vector<N + 1, T>, N> add_dimension(const std::array<Vector<N, T>, N>& a, RandomEngine& engine)
{
        std::uniform_int_distribution<unsigned> uid(0, N);
        std::array<Vector<N + 1, T>, N> res;
        const std::size_t k = uid(engine);
        for (std::size_t n = 0; n < N; ++n)
        {
                for (std::size_t i = 0; i < k; ++i)
                {
                        res[n][i] = a[n][i];
                }
                res[n][k] = 0;
                for (std::size_t i = k + 1; i < N + 1; ++i)
                {
                        res[n][i] = a[n][i - 1];
                }
        }
        return res;
}

template <typename T>
void test_sphere_1_simplex(const T precision)
{
        LOG(std::string("Test sphere 1-simplex, ") + type_name<T>());

        PCG engine;

        const auto cmp = [&](const T v1, const T v2)
        {
                compare("Test sphere 1-simplex", v1, v2, precision);
        };

        const auto add = [&](const auto& a)
        {
                return add_dimension(a, engine);
        };

        // arcLength[a_, b_] :=
        //   Module[{an, bn, cn, dihedralA, dihedralB, dihedralC},
        //    an = Normalize[a];
        //    bn = Normalize[b];
        //    ArcCos[Dot[an, bn]]];
        // printVector[i_, v_] :=
        //   Print[StringTemplate["v[``] = {``, ``};"][i, v[[1]], v[[2]]]];
        // SeedRandom[ToString[N[Pi, 1000]]];
        // For[i = 0, i < 10, ++i, Module[{a, b},
        //   a = RandomInteger[{-10, 10}, 2];
        //   b = RandomInteger[{-10, 10}, 2];
        //   printVector[0, a];
        //   printVector[1, b];
        //   Print[StringTemplate["cmp(sphere_simplex_area(v), ``L);"]
        //     [N[arcLength[a, b], 50]]]]]

        std::array<Vector<2, T>, 2> v;

        v[0] = {-4, -9};
        v[1] = {2, 1};
        cmp(sphere_simplex_area(v), 2.4526682653749318366367497521279659064762337027082L);
        v[0] = {-8, 1};
        v[1] = {10, 4};
        cmp(sphere_simplex_area(v), 2.6367312819306669171277006173051987541265934939472L);
        v[0] = {9, -5};
        v[1] = {0, -5};
        cmp(sphere_simplex_area(v), 1.0636978224025596609438911160525454785625629654193L);
        v[0] = {3, -2};
        v[1] = {3, 10};
        cmp(sphere_simplex_area(v), 1.8673421358645970784813281508319456761971243467314L);
        v[0] = {-6, -4};
        v[1] = {7, -4};
        cmp(sphere_simplex_area(v), 2.0344439357957027354455779231009658441271217539737L);
        v[0] = {-2, 0};
        v[1] = {-1, -9};
        cmp(sphere_simplex_area(add(v)), 1.4601391056210009726721818194296893361232986046845L);
        v[0] = {-4, 1};
        v[1] = {-8, -10};
        cmp(sphere_simplex_area(add(v)), 1.141034047698208110346883199241213637938722543228L);
        v[0] = {-6, 7};
        v[1] = {-8, 6};
        cmp(sphere_simplex_area(add(v)), 0.21866894587394196204217375024993859111439295590494L);
        v[0] = {7, 10};
        v[1] = {3, 5};
        cmp(sphere_simplex_area(add(v)), 0.070306464118624461100180511881636183683354814666235L);
        v[0] = {9, -10};
        v[1] = {-2, 4};
        cmp(sphere_simplex_area(add(v)), 2.872425160804092763036107542006437034239950774303L);
}

template <typename T>
void test_sphere_2_simplex(const T precision)
{
        LOG(std::string("Test sphere 2-simplex, ") + type_name<T>());

        PCG engine;

        const auto cmp = [&](const T v1, const T v2)
        {
                compare("Test sphere 2-simplex", v1, v2, precision);
        };

        const auto add = [&](const auto& a)
        {
                return add_dimension(a, engine);
        };

        // triangleArea[a_, b_, c_] :=
        //   Module[{an, bn, cn, dihedralA, dihedralB, dihedralC},
        //    an = Normalize[a];
        //    bn = Normalize[b];
        //    cn = Normalize[c];
        //    dihedralA =
        //     ArcCos[Dot[Normalize[Cross[an, bn]], Normalize[Cross[an, cn]]]];
        //    dihedralB =
        //     ArcCos[Dot[Normalize[Cross[bn, cn]], Normalize[Cross[bn, an]]]];
        //    dihedralC =
        //     ArcCos[Dot[Normalize[Cross[cn, an]], Normalize[Cross[cn, bn]]]];
        //    dihedralA + dihedralB + dihedralC - Pi];
        // printVector[i_, v_] :=
        //   Print[StringTemplate["v[``] = {``, ``, ``};"][i, v[[1]], v[[2]], v[[3]]]];
        // SeedRandom[ToString[N[Pi, 1000]]];
        // For[i = 0, i < 10, ++i, Module[{a, b, c},
        //   a = RandomInteger[{-10, 10}, 3];
        //   b = RandomInteger[{-10, 10}, 3];
        //   c = RandomInteger[{-10, 10}, 3];
        //   printVector[0, a];
        //   printVector[1, b];
        //   printVector[2, c];
        //   Print[StringTemplate["cmp(sphere_simplex_area(v), ``L);"]
        //     [N[triangleArea[a, b, c], 50]]]]]

        std::array<Vector<3, T>, 3> v;

        v[0] = {-4, -9, 2};
        v[1] = {1, -8, 1};
        v[2] = {10, 4, 9};
        cmp(sphere_simplex_area(v), 0.58894016415276939679712917287183832189081988395548L);
        v[0] = {-5, 0, -5};
        v[1] = {3, -2, 3};
        v[2] = {10, -6, -4};
        cmp(sphere_simplex_area(v), 1.8056355397062009515401064992048278775175747735778L);
        v[0] = {7, -4, -2};
        v[1] = {0, -1, -9};
        v[2] = {-4, 1, -8};
        cmp(sphere_simplex_area(v), 0.025758859392057396369107902542460771041479991507537L);
        v[0] = {-10, -6, 7};
        v[1] = {-8, 6, 7};
        v[2] = {10, 3, 5};
        cmp(sphere_simplex_area(v), 1.3818711567348440784918145212810345861294425238664L);
        v[0] = {9, -10, -2};
        v[1] = {4, -10, -4};
        v[2] = {7, -3, 6};
        cmp(sphere_simplex_area(v), 0.11325417941462770399714843181416433092790453583601L);
        v[0] = {7, 0, 1};
        v[1] = {1, -7, 7};
        v[2] = {5, 10, 0};
        cmp(sphere_simplex_area(add(v)), 0.98252612604983535516148507772664202158065955945576L);
        v[0] = {-7, 10, 0};
        v[1] = {0, -5, 3};
        v[2] = {-6, -7, 0};
        cmp(sphere_simplex_area(add(v)), 1.2376235869391378346758735075338889682631592176817L);
        v[0] = {-4, 2, 3};
        v[1] = {-3, -5, 3};
        v[2] = {8, 9, 0};
        cmp(sphere_simplex_area(add(v)), 2.2407216398660339591633056398278295908733141107547L);
        v[0] = {-8, 7, 7};
        v[1] = {9, -9, -4};
        v[2] = {-8, 10, 10};
        cmp(sphere_simplex_area(add(v)), 0.86092298564120217220950178057093316846752603538137L);
        v[0] = {-6, 3, 0};
        v[1] = {0, -8, -3};
        v[2] = {-3, -6, 7};
        cmp(sphere_simplex_area(add(v)), 1.5028890943060527292884260512774281120039285983927L);
}

void test_1()
{
        test_sphere_1_simplex<float>(1e-3);
        test_sphere_1_simplex<double>(1e-12);
        test_sphere_1_simplex<long double>(1e-16);
}

void test_2()
{
        test_sphere_2_simplex<float>(1e-3);
        test_sphere_2_simplex<double>(1e-12);
        test_sphere_2_simplex<long double>(1e-16);
}

void test_integrate_1_simplex(progress::Ratio* const progress)
{
        test_integrate<2, float>(progress, 0, 0.5);
        test_integrate<2, double>(progress, 0.5, 1.0);
}

void test_integrate_2_simplex(progress::Ratio* const progress)
{
        test_integrate<3, float>(progress, 0, 0.5);
        test_integrate<3, double>(progress, 0.5, 1);
}

TEST_SMALL("Sphere 1-Simplex", test_1)
TEST_SMALL("Sphere 2-Simplex", test_2)

TEST_SMALL("Sphere 1-Simplex, Integrate", test_integrate_1_simplex)
TEST_SMALL("Sphere 2-Simplex, Integrate", test_integrate_2_simplex)
}
}
