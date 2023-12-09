/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "parallelotope_tests.h"

#include "../hyperplane_parallelotope.h"
#include "../parallelotope.h"
#include "../parallelotope_aa.h"
#include "../shape_overlap.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <memory>
#include <string>

namespace ns::geometry::spatial::test
{
namespace
{
template <std::size_t N, typename T>
constexpr Vector<N, T> last_coord_vector(const T& v, const T& last)
{
        static_assert(N > 0);
        Vector<N, T> res;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res[i] = v;
        }
        res[N - 1] = last;
        return res;
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N + 1, T>, N> to_edge_vector_hyper(const std::array<T, N>& edges)
{
        std::array<Vector<N + 1, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < N + 1; ++j)
                {
                        res[i][j] = 0;
                }
                res[i][i] = edges[i];
        }
        return res;
}

template <typename Parallelotope1, typename Parallelotope2>
void test_overlap(const Parallelotope1& p1, const Parallelotope2& p2, const bool overlap, const std::string& text)
{
        if (overlap != shapes_overlap(p1, p2))
        {
                error("Error overlap " + text);
        }
}

template <typename Parallelotope>
std::unique_ptr<const ShapeOverlap<Parallelotope>> make_shape_overlap(const Parallelotope* const p)
{
        return std::make_unique<ShapeOverlap<Parallelotope>>(p);
}

template <std::size_t N, typename T>
void test_overlap()
{
        const std::string name = "Test parallelotope overlap in " + space_name(N);

        constexpr std::array<T, N> EDGES = make_array_value<T, N>(1);
        constexpr Vector<N, T> ORG_0(0.0);
        constexpr Vector<N, T> ORG_1(0.75);
        constexpr Vector<N, T> ORG_2(1.5);

        constexpr Vector<N, T> ORG_BIG(-5);
        constexpr std::array<T, N> EDGES_BIG = make_array_value<T, N>(10);

        LOG("------------------------------");
        LOG(name);

        LOG("ParallelotopeAA");

        {
                const ParallelotopeAA<N, T> p1(ORG_0, EDGES);
                const ParallelotopeAA<N, T> p2(ORG_1, EDGES);
                const ParallelotopeAA<N, T> p3(ORG_2, EDGES);
                const ParallelotopeAA<N, T> p_big(ORG_BIG, EDGES_BIG);

                const std::unique_ptr w1 = make_shape_overlap(&p1);
                const std::unique_ptr w2 = make_shape_overlap(&p2);
                const std::unique_ptr w3 = make_shape_overlap(&p3);
                const std::unique_ptr w_big = make_shape_overlap(&p_big);

                test_overlap(*w1, *w2, true, "1-2");
                test_overlap(*w2, *w3, true, "2-3");
                test_overlap(*w1, *w3, false, "1-3");

                test_overlap(*w1, *w_big, true, "1-big");
                test_overlap(*w2, *w_big, true, "2-big");
                test_overlap(*w3, *w_big, true, "3-big");
        }

        LOG("Parallelotope");

        {
                const Parallelotope<N, T> p1(ORG_0, EDGES);
                const Parallelotope<N, T> p2(ORG_1, EDGES);
                const Parallelotope<N, T> p3(ORG_2, EDGES);
                const Parallelotope<N, T> p_big(ORG_BIG, EDGES_BIG);

                const std::unique_ptr w1 = make_shape_overlap(&p1);
                const std::unique_ptr w2 = make_shape_overlap(&p2);
                const std::unique_ptr w3 = make_shape_overlap(&p3);
                const std::unique_ptr w_big = make_shape_overlap(&p_big);

                test_overlap(*w1, *w2, true, "1-2");
                test_overlap(*w2, *w3, true, "2-3");
                test_overlap(*w1, *w3, false, "1-3");

                test_overlap(*w1, *w_big, true, "1-big");
                test_overlap(*w2, *w_big, true, "2-big");
                test_overlap(*w3, *w_big, true, "3-big");
        }

        LOG(name + " passed");
}

template <std::size_t N, typename T>
void test_overlap_hyperplane()
{
        const std::string name = "Test hyperplane parallelotope overlap in " + space_name(N);

        constexpr Vector<N, T> ORG(5);
        constexpr T SIZE = 1;

        constexpr T SIZE_BIG = 3;

        constexpr Vector<N, T> BIG_1 = last_coord_vector<N, T>(4.0, 4.9);
        constexpr Vector<N, T> BIG_2 = last_coord_vector<N, T>(4.0, 5.5);
        constexpr Vector<N, T> BIG_3 = last_coord_vector<N, T>(4.0, 6.1);

        constexpr T SIZE_SMALL = 0.2;

        constexpr Vector<N, T> SMALL_1 = last_coord_vector<N, T>(4.9, 4.9);
        constexpr Vector<N, T> SMALL_2 = last_coord_vector<N, T>(4.9, 5.5);
        constexpr Vector<N, T> SMALL_3 = last_coord_vector<N, T>(4.9, 6.1);

        constexpr Vector<N, T> SMALL_4 = last_coord_vector<N, T>(4.0, 4.9);
        constexpr Vector<N, T> SMALL_5 = last_coord_vector<N, T>(4.0, 5.5);
        constexpr Vector<N, T> SMALL_6 = last_coord_vector<N, T>(4.0, 6.1);

        constexpr Vector<N, T> SMALL_7 = last_coord_vector<N, T>(5.5, 4.9);
        constexpr Vector<N, T> SMALL_8 = last_coord_vector<N, T>(5.5, 5.5);
        constexpr Vector<N, T> SMALL_9 = last_coord_vector<N, T>(5.5, 6.1);

        LOG("------------------------------");
        LOG(name);

        constexpr std::array<Vector<N, T>, N - 1> EDGES_HYPER_BIG =
                to_edge_vector_hyper(make_array_value<T, N - 1>(SIZE_BIG));

        constexpr std::array<Vector<N, T>, N - 1> EDGES_HYPER_SMALL =
                to_edge_vector_hyper(make_array_value<T, N - 1>(SIZE_SMALL));

        const HyperplaneParallelotope<N, T> p1(BIG_1, EDGES_HYPER_BIG);
        const HyperplaneParallelotope<N, T> p2(BIG_2, EDGES_HYPER_BIG);
        const HyperplaneParallelotope<N, T> p3(BIG_3, EDGES_HYPER_BIG);
        const HyperplaneParallelotope<N, T> p4(SMALL_1, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p5(SMALL_2, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p6(SMALL_3, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p7(SMALL_4, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p8(SMALL_5, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p9(SMALL_6, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p10(SMALL_7, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p11(SMALL_8, EDGES_HYPER_SMALL);
        const HyperplaneParallelotope<N, T> p12(SMALL_9, EDGES_HYPER_SMALL);

        const std::unique_ptr w1 = make_shape_overlap(&p1);
        const std::unique_ptr w2 = make_shape_overlap(&p2);
        const std::unique_ptr w3 = make_shape_overlap(&p3);
        const std::unique_ptr w4 = make_shape_overlap(&p4);
        const std::unique_ptr w5 = make_shape_overlap(&p5);
        const std::unique_ptr w6 = make_shape_overlap(&p6);
        const std::unique_ptr w7 = make_shape_overlap(&p7);
        const std::unique_ptr w8 = make_shape_overlap(&p8);
        const std::unique_ptr w9 = make_shape_overlap(&p9);
        const std::unique_ptr w10 = make_shape_overlap(&p10);
        const std::unique_ptr w11 = make_shape_overlap(&p11);
        const std::unique_ptr w12 = make_shape_overlap(&p12);

        const auto test = [&](const auto& parallelotope)
        {
                const std::unique_ptr w = make_shape_overlap(&parallelotope);

                test_overlap(*w1, *w, false, "1-p");
                test_overlap(*w2, *w, true, "2-p");
                test_overlap(*w3, *w, false, "3-p");

                test_overlap(*w4, *w, false, "4-p");
                test_overlap(*w5, *w, true, "5-p");
                test_overlap(*w6, *w, false, "6-p");

                test_overlap(*w7, *w, false, "7-p");
                test_overlap(*w8, *w, false, "8-p");
                test_overlap(*w9, *w, false, "9-p");

                test_overlap(*w10, *w, false, "10-p");
                test_overlap(*w11, *w, true, "11-p");
                test_overlap(*w12, *w, false, "12-p");
        };

        constexpr std::array<T, N> EDGES = make_array_value<T, N>(SIZE);

        LOG("ParallelotopeAA");
        test(ParallelotopeAA<N, T>(ORG, EDGES));

        LOG("Parallelotope");
        test(Parallelotope<N, T>(ORG, EDGES));

        LOG(name + " passed");
}

template <std::size_t N, typename T>
void all_tests()
{
        constexpr int POINT_COUNT = 5'000;

        test_points<N, T>(POINT_COUNT);
        test_algorithms<N, T>();
        test_overlap<N, T>();
        test_overlap_hyperplane<N, T>();
}

template <typename T>
void all_tests()
{
        all_tests<2, T>();
        all_tests<3, T>();
        all_tests<4, T>();
        all_tests<5, T>();
}

void test_parallelotope()
{
        all_tests<float>();
        all_tests<double>();
}

TEST_SMALL("Parallelotope", test_parallelotope)
}
}
