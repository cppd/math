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

#include "surface_test.h"

#include "points.h"

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/random.h"
#include "geometry_cocone/surface.h"
#include "progress/progress.h"

#include <cmath>
#include <random>

namespace
{
template <size_t N>
void test(const std::vector<Vector<N, float>>& points, size_t expected_facet_count, size_t expected_bound_facet_count)
{
        ASSERT(points.size() > N);
        ASSERT(expected_facet_count > 0);
        ASSERT(expected_bound_facet_count > 0);

        LOG("Point count: " + to_string(points.size()));
        LOG("Expected facet count: " + to_string(expected_facet_count));
        LOG("Expected bound facet count: " + to_string(expected_bound_facet_count));

        ProgressRatio progress(nullptr);

        std::unique_ptr<ISurfaceConstructor<N>> sr = create_surface_constructor(points, &progress);

        std::vector<Vector<N, double>> normals;
        std::vector<std::array<int, N>> facets;

        sr->cocone(&normals, &facets, &progress);

        LOG("COCONE facet count: " + to_string(facets.size()));
        if (facets.size() != expected_facet_count)
        {
                error("Error facet count: expected " + to_string(expected_facet_count) + ", COCONE computed " +
                      to_string(facets.size()));
        }

        sr->bound_cocone(0.3, 0.14, &normals, &facets, &progress);

        LOG("BOUND COCONE facet count: " + to_string(facets.size()));
        double ratio = static_cast<double>(facets.size()) / expected_bound_facet_count;
        if (!(ratio > 0.9 && ratio < 1.1))
        {
                error("Error bound facet count: expected " + to_string(expected_bound_facet_count) + ", BOUND COCONE computed " +
                      to_string(facets.size()));
        }

        LOG("check passed");
}

template <size_t N>
void all_tests(size_t size)
{
        std::vector<Vector<N, float>> points;
        size_t expected_facet_count, expected_bound_facet_count;

        LOG("-------" + to_string(N) + "D-------");
        generate_points_ellipsoid<N>(size, &points, &expected_facet_count, &expected_bound_facet_count);
        test(points, expected_facet_count, expected_bound_facet_count);

        LOG("-------" + to_string(N) + "D-------");
        generate_points_object_recess<N>(size, &points, &expected_facet_count, &expected_bound_facet_count);
        test(points, expected_facet_count, expected_bound_facet_count);
}
}

void surface_test()
{
        try
        {
                std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

                int size;

                size = std::uniform_int_distribution<int>(100, 200)(engine);
                all_tests<2>(size);

                size = std::uniform_int_distribution<int>(1000, 2000)(engine);
                all_tests<3>(size);

                LOG("");

                // std::exit(EXIT_SUCCESS);
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("surface test error: ") + e.what());
        }
}
