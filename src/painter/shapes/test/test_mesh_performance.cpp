/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "spherical_mesh.h"

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T, typename Color>
bool intersections(const Scene<N, T, Color>& scene, Ray<N, T> ray)
{
        static constexpr std::optional<Vector<N, T>> EMPTY_GEOMETRIC_NORMAL;

        const SurfacePoint surface_1 = scene.intersect(EMPTY_GEOMETRIC_NORMAL, ray);
        if (!surface_1)
        {
                return false;
        }
        ray.set_org(surface_1.point());
        return scene.intersect(surface_1.geometric_normal(), ray);
}

template <std::size_t N, typename T, typename Color>
void test(const test::SphericalMesh<N, T, Color>& mesh, const std::vector<Ray<N, T>>& rays)
{
        static constexpr std::size_t GROUP_SIZE = 0x1000;

        const auto f = [&]
        {
                for (std::size_t i = 0; i < rays.size();)
                {
                        MemoryArena::thread_local_instance().clear();

                        const std::size_t r = rays.size() - i;
                        const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                        for (; i < c; ++i)
                        {
                                do_not_optimize(intersections(*mesh.scene, rays[i]));
                        }
                }
        };

        const long long start_ray_count = mesh.scene->thread_ray_count();
        const Clock::time_point start_time = Clock::now();

        for (int i = 0; i < 10; ++i)
        {
                f();
        }

        const double duration = duration_from(start_time);
        const long long ray_count = mesh.scene->thread_ray_count() - start_ray_count;

        std::string s;
        s += "Mesh intersections <" + to_string(N) + ", " + type_name<T>() + ">: ";
        s += to_string_digit_groups(mesh.facet_count) + " facets, "
             + to_string_digit_groups(std::llround(ray_count / duration)) + " o/s";
        LOG(s);
}

template <std::size_t N, typename T>
void test(const int point_count, const int ray_count, ProgressRatio* const progress)
{
        using Color = color::Spectrum;

        PCG engine;

        test::SphericalMesh<N, T, Color> mesh =
                test::create_spherical_mesh_scene<N, T, Color>(point_count, engine, progress);

        test(mesh, test::create_spherical_mesh_center_rays(mesh.bounding_box, ray_count, engine));
}

template <std::size_t N>
void test(const int point_count, const int ray_count, ProgressRatio* const progress)
{
        test<N, float>(point_count, ray_count, progress);
        test<N, double>(point_count, ray_count, progress);
}

void test_performance(ProgressRatio* const progress)
{
        test<3>(150'000, 50'000, progress);
        test<4>(40'000, 20'000, progress);
        test<5>(10'000, 5'000, progress);
        test<6>(2'000, 500, progress);
}

TEST_PERFORMANCE("Mesh intersections", test_performance)
}
}
