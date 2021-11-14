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

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/test/test.h>

#include <random>
#include <sstream>
#include <vector>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T, typename Color>
std::tuple<T, const Surface<N, T, Color>*> intersect_mesh(
        const Shape<N, T, Color>& mesh,
        const Ray<N, T>& ray,
        int* const error_count)
{
        const std::optional<T> bounds_distance = mesh.intersect_bounds(ray, Limits<T>::max());
        if (!bounds_distance)
        {
                ++(*error_count);
                return {0, nullptr};
        }

        const auto [distance, surface] = mesh.intersect(ray, Limits<T>::max(), *bounds_distance);
        if (!surface)
        {
                ++(*error_count);
                return {0, nullptr};
        }

        return {distance, surface};
}

template <std::size_t N, typename T, typename Color>
void intersect_mesh(const Shape<N, T, Color>& mesh, Ray<N, T> ray, const T ray_offset, int* const error_count)
{
        {
                const auto [distance, surface] = intersect_mesh(mesh, ray, error_count);
                if (!surface)
                {
                        return;
                }

                ray.set_org(surface->point(ray, distance));
                ray.move(ray_offset);
        }
        {
                const auto [distance, surface] = intersect_mesh(mesh, ray, error_count);
                if (!surface)
                {
                        return;
                }

                ray.set_org(surface->point(ray, distance));
                ray.move(ray_offset);
        }

        const std::optional<T> bounds_distance = mesh.intersect_bounds(ray, Limits<T>::max());
        if (bounds_distance)
        {
                const auto [distance, surface] = mesh.intersect(ray, Limits<T>::max(), *bounds_distance);
                if (surface)
                {
                        ++(*error_count);
                        return;
                }
        }
}

void check_intersections(const int ray_count, const int error_count)
{
        const double relative_error = static_cast<double>(error_count) / ray_count;

        std::ostringstream oss;
        oss << std::scientific << std::setprecision(3);
        oss << "error count = " << error_count << ", ray count = " << ray_count
            << ", relative error = " << relative_error;
        LOG(oss.str());

        if (!(relative_error < 5e-5))
        {
                error("Too many errors, " + oss.str());
        }
}

template <std::size_t N, typename T, typename Color>
void test_spherical_mesh(
        const Shape<N, T, Color>& mesh,
        const std::vector<Ray<N, T>>& rays,
        ProgressRatio* const progress)
{
        constexpr std::size_t GROUP_SIZE = 0x1000;

        const double rays_size_reciprocal = 1.0 / rays.size();

        const geometry::BoundingBox<N, T> bb = mesh.bounding_box();
        const T ray_offset =
                std::max(bb.min().norm_infinity(), bb.max().norm_infinity()) * (100 * Limits<T>::epsilon());
        LOG("ray offset = " + to_string(ray_offset));

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
                        intersect_mesh(mesh, rays[i], ray_offset, &error_count);
                }
                progress->set(i * rays_size_reciprocal);
        }
        LOG("intersections " + to_string_fixed(duration_from(start_time), 5) + " s");

        check_intersections(rays.size(), error_count);
}

template <std::size_t N, typename T>
void test_mesh(
        const int point_low,
        const int point_high,
        const int ray_low,
        const int ray_high,
        ProgressRatio* const progress)
{
        using Color = color::Spectrum;

        ASSERT(point_low <= point_high);
        ASSERT(ray_low <= ray_high);

        LOG("----------- " + space_name(N) + ", " + type_name<T>() + " -----------");

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();

        const int point_count = std::uniform_int_distribution<int>(point_low, point_high)(random_engine);
        std::unique_ptr<const Shape<N, T, Color>> mesh =
                test::create_spherical_mesh<N, T, Color>(point_count, random_engine, progress);

        const int ray_count = std::uniform_int_distribution<int>(ray_low, ray_high)(random_engine);
        const std::vector<Ray<N, T>> rays =
                test::create_rays_for_spherical_mesh(mesh->bounding_box(), ray_count, random_engine);

        test_spherical_mesh(*mesh, rays, progress);
}

//

void test_mesh_3(ProgressRatio* const progress)
{
        test_mesh<3, float>(500, 1000, 90'000, 110'000, progress);
        LOG("");
        test_mesh<3, double>(500, 1000, 90'000, 110'000, progress);
}

void test_mesh_4(ProgressRatio* const progress)
{
        test_mesh<4, float>(500, 1000, 90'000, 110'000, progress);
        LOG("");
        test_mesh<4, double>(500, 1000, 90'000, 110'000, progress);
}

void test_mesh_5(ProgressRatio* const progress)
{
        test_mesh<5, float>(1000, 2000, 90'000, 110'000, progress);
        LOG("");
        test_mesh<5, double>(1000, 2000, 90'000, 110'000, progress);
}

void test_mesh_6(ProgressRatio* const progress)
{
        test_mesh<6, float>(1000, 2000, 90'000, 110'000, progress);
        LOG("");
        test_mesh<6, double>(1000, 2000, 90'000, 110'000, progress);
}

TEST_SMALL("Mesh Intersections, 3-Space", test_mesh_3)
TEST_SMALL("Mesh Intersections, 4-Space", test_mesh_4)
TEST_LARGE("Mesh Intersections, 5-Space", test_mesh_5)
TEST_LARGE("Mesh Intersections, 6-Space", test_mesh_6)
}
}
