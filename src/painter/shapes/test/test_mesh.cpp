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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <vector>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T>
constexpr std::optional<Vector<N, T>> EMPTY_GEOMETRIC_NORMAL;

constexpr std::size_t GROUP_SIZE = 0x1000;

template <std::size_t N, typename T, typename Color>
bool intersections(const Scene<N, T, Color>& scene, Ray<N, T> ray)
{
        const SurfacePoint surface_1 = scene.intersect(EMPTY_GEOMETRIC_NORMAL<N, T>, ray);
        if (!surface_1)
        {
                return false;
        }
        ray.set_org(surface_1.point());

        const SurfacePoint surface_2 = scene.intersect(surface_1.geometric_normal(), ray);
        if (!surface_2)
        {
                return false;
        }
        ray.set_org(surface_2.point());

        return !scene.intersect(surface_2.geometric_normal(), ray);
}

template <std::size_t N, typename T, typename Color>
void test_intersections(
        const test::SphericalMesh<N, T, Color>& mesh,
        const std::vector<Ray<N, T>>& rays,
        ProgressRatio* const progress)
{
        const double rays_size_reciprocal = 1.0 / rays.size();

        progress->set(0);
        progress->set_text(std::string("Ray intersections, ") + type_name<T>());

        int error_count = 0;

        for (std::size_t i = 0; i < rays.size();)
        {
                MemoryArena::thread_local_instance().clear();

                const std::size_t r = rays.size() - i;
                const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                for (; i < c; ++i)
                {
                        if (!intersections(*mesh.scene, rays[i]))
                        {
                                ++error_count;
                        }
                }
                progress->set(i * rays_size_reciprocal);
        }

        std::string s;
        s += "error count = " + to_string_digit_groups(error_count);
        s += ", ray count = " + to_string_digit_groups(rays.size());
        if (!(error_count <= std::lround(rays.size() * 4e-5)))
        {
                error("Too many intersection errors, " + s);
        }
        LOG(s);
}

template <std::size_t N, typename T, typename Color>
void test_surface_ratio(
        const test::SphericalMesh<N, T, Color>& mesh,
        const std::vector<Ray<N, T>>& rays,
        ProgressRatio* const progress)
{
        const double rays_size_reciprocal = 1.0 / rays.size();

        progress->set(0);
        progress->set_text(std::string("Ray intersections, ") + type_name<T>());

        int mesh_intersections = 0;

        for (std::size_t i = 0; i < rays.size();)
        {
                MemoryArena::thread_local_instance().clear();

                const std::size_t r = rays.size() - i;
                const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                for (; i < c; ++i)
                {
                        if (mesh.scene->intersect(EMPTY_GEOMETRIC_NORMAL<N, T>, rays[i]))
                        {
                                ++mesh_intersections;
                        }
                }
                progress->set(i * rays_size_reciprocal);
        }

        const T intersection_ratio = static_cast<T>(mesh_intersections) / rays.size();
        const T surface_ratio = mesh.surface / mesh.bounding_box.surface();

        const T relative_error = std::abs(intersection_ratio - surface_ratio)
                                 / std::max(std::abs(intersection_ratio), std::abs(surface_ratio));

        std::string s;
        s += "intersection ratio = " + to_string(intersection_ratio);
        s += ", area ratio = " + to_string(surface_ratio);
        if (!(relative_error < 0.05))
        {
                error("Intersection error, " + s + ", relative error " + to_string(relative_error));
        }
        LOG(s);
}

template <std::size_t N, typename T>
void test(const int point_count, const int ray_count, ProgressRatio* const progress)
{
        using Color = color::Spectrum;

        const std::string name = "Test mesh intersections, " + space_name(N) + ", " + type_name<T>();

        LOG(name);

        PCG engine;

        test::SphericalMesh<N, T, Color> mesh =
                test::create_spherical_mesh_scene<N, T, Color>(point_count, engine, progress);

        test_intersections(
                mesh, test::create_spherical_mesh_center_rays(mesh.bounding_box, ray_count, engine), progress);

        test_surface_ratio(
                mesh, test::create_random_intersections_rays(mesh.bounding_box, ray_count, engine), progress);

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
