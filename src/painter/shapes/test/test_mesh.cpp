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

#include "spherical_mesh.h"

#include "../../scenes/storage_scene.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/test/test.h>

#include <random>
#include <vector>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T, typename Color>
bool intersections(const Scene<N, T, Color>& scene, Ray<N, T> ray)
{
        for (int i = 0; i < 2; ++i)
        {
                const SurfacePoint surface = scene.intersect(ray);
                if (!surface)
                {
                        return false;
                }
                ray.set_org(surface.point());
        }
        return !scene.intersect(ray);
}

void check_intersections(const int ray_count, const int error_count)
{
        std::string s;
        s += "error count = " + to_string_digit_groups(error_count);
        s += ", ray count = " + to_string_digit_groups(ray_count);
        LOG(s);

        if (!(error_count <= std::lround(ray_count * 4e-5)))
        {
                error("Too many intersection errors, " + s);
        }
}

template <std::size_t N, typename T, typename Color>
void test(const Scene<N, T, Color>& scene, const std::vector<Ray<N, T>>& rays, ProgressRatio* const progress)
{
        constexpr std::size_t GROUP_SIZE = 0x1000;

        const double rays_size_reciprocal = 1.0 / rays.size();

        LOG("intersections...");
        progress->set_text(std::string("Ray intersections, ") + type_name<T>());

        int error_count = 0;

        const Clock::time_point start_time = Clock::now();

        for (std::size_t i = 0; i < rays.size();)
        {
                MemoryArena::thread_local_instance().clear();

                const std::size_t r = rays.size() - i;
                const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                for (; i < c; ++i)
                {
                        if (!intersections(scene, rays[i]))
                        {
                                ++error_count;
                        }
                }
                progress->set(i * rays_size_reciprocal);
        }

        const double duration = duration_from(start_time);
        check_intersections(rays.size(), error_count);
        LOG(to_string_digit_groups(std::llround(scene.thread_ray_count() / duration)) + " intersections per second");
}

template <std::size_t N, typename T>
void test(const int point_count, const int ray_count, ProgressRatio* const progress)
{
        using Color = color::Spectrum;

        const std::string name = "Test mesh intersections, " + space_name(N) + ", " + type_name<T>();

        LOG(name);

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        std::unique_ptr<const Shape<N, T, Color>> mesh =
                test::create_spherical_mesh<N, T, Color>(point_count, random_engine, progress);

        const std::vector<Ray<N, T>> rays =
                test::create_rays_for_spherical_mesh(mesh->bounding_box(), ray_count, random_engine);

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> meshes;
        meshes.push_back(std::move(mesh));

        const std::unique_ptr<const Scene<N, T, Color>> scene =
                create_storage_scene(Color(), {}, {}, std::move(meshes), progress);

        test(*scene, rays, progress);

        LOG(name + " passed");
}

template <std::size_t N>
void test_mesh(const int point_count, const int ray_count, ProgressRatio* const progress)
{
        test<N, float>(point_count, ray_count, progress);
        test<N, double>(point_count, ray_count, progress);
}

void test_mesh_3(ProgressRatio* const progress)
{
        test_mesh<3>(1000, 100'000, progress);
}

void test_mesh_4(ProgressRatio* const progress)
{
        test_mesh<4>(1000, 100'000, progress);
}

void test_mesh_5(ProgressRatio* const progress)
{
        test_mesh<5>(2000, 100'000, progress);
}

void test_mesh_6(ProgressRatio* const progress)
{
        test_mesh<6>(2000, 100'000, progress);
}

TEST_SMALL("Mesh Intersections, 3-Space", test_mesh_3)
TEST_SMALL("Mesh Intersections, 4-Space", test_mesh_4)
TEST_LARGE("Mesh Intersections, 5-Space", test_mesh_5)
TEST_LARGE("Mesh Intersections, 6-Space", test_mesh_6)
}
}
