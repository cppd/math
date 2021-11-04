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

#include "../mesh.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/geometry/core/convex_hull.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <tuple>
#include <vector>

namespace ns::painter
{
namespace
{
constexpr bool WITH_RAY_LOG = false;
constexpr bool WITH_ERROR_LOG = false;

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_rays_for_spherical_mesh(const geometry::BoundingBox<N, T>& bb, const int ray_count)
{
        const Vector<N, T> center = bb.center();
        const T radius = 2 * (bb.diagonal() / T(2)).norm_infinity();

        LOG("ray center = " + to_string(center));
        LOG("ray radius = " + to_string(radius));

        std::mt19937_64 random_engine(ray_count);
        std::vector<Ray<N, T>> rays;
        rays.resize(ray_count);
        for (Ray<N, T>& ray : rays)
        {
                Vector<N, T> v = sampling::uniform_on_sphere<N, T>(random_engine);
                ray = Ray<N, T>(radius * v + center, -v);
        }
        return rays;
}

template <std::size_t N, typename T, typename Color>
ShapeIntersection<N, T, Color> intersect_mesh(
        const char* const name_1,
        const char* const name_2,
        const Shape<N, T, Color>& mesh,
        const Ray<N, T>& ray,
        const unsigned ray_number,
        int* const error_count)
{
        const std::optional<T> bounding_distance = mesh.intersect_bounding(ray);
        if (!bounding_distance)
        {
                if (WITH_ERROR_LOG)
                {
                        LOG(std::string("No ") + name_1 + " bounding intersection with ray #" + to_string(ray_number)
                            + "\n" + to_string(ray));
                }
                ++(*error_count);
                return ShapeIntersection<N, T, Color>(nullptr);
        }
        if (WITH_RAY_LOG)
        {
                LOG(std::string(name_2) + "_a == " + to_string(*bounding_distance));
        }

        const ShapeIntersection<N, T, Color> intersection = mesh.intersect(ray, *bounding_distance);
        if (!intersection.surface)
        {
                if (WITH_ERROR_LOG)
                {
                        LOG(std::string("No ") + name_1 + " intersection with ray #" + to_string(ray_number) + "\n"
                            + "bounding distance " + to_string(*bounding_distance) + "\n" + to_string(ray));
                }
                ++(*error_count);
                return ShapeIntersection<N, T, Color>(nullptr);
        }
        if (WITH_RAY_LOG)
        {
                LOG(std::string(name_2) + "_p == " + to_string(intersection.distance));
        }

        return intersection;
}

template <std::size_t N, typename T, typename Color>
void test_spherical_mesh(const Shape<N, T, Color>& mesh, const int ray_count, ProgressRatio* const progress)
{
        const geometry::BoundingBox<N, T> bb = mesh.bounding_box();

        const T ray_offset =
                std::max(bb.min().norm_infinity(), bb.max().norm_infinity()) * (100 * Limits<T>::epsilon());
        LOG("ray offset = " + to_string(ray_offset));

        const std::vector<Ray<N, T>> rays = create_rays_for_spherical_mesh(bb, ray_count);

        int error_count = 0;

        LOG("intersections...");
        progress->set_text("Rays: %v of %m");

        const Clock::time_point start_time = Clock::now();

        for (unsigned i = 1; i <= rays.size(); ++i)
        {
                MemoryArena::thread_local_instance().clear();

                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(i, rays.size());
                }

                Ray<N, T> ray = rays[i - 1];

                if (WITH_RAY_LOG)
                {
                        LOG("");
                        LOG("ray #" + to_string(i) + " in " + space_name(N));
                }

                ShapeIntersection<N, T, Color> intersection;

                intersection = intersect_mesh("the first", "t1", mesh, ray, i, &error_count);
                if (!intersection.surface)
                {
                        continue;
                }

                ray.set_org(intersection.surface->point());
                ray.move(ray_offset);

                intersection = intersect_mesh("the second", "t2", mesh, ray, i, &error_count);
                if (!intersection.surface)
                {
                        continue;
                }

                ray.set_org(intersection.surface->point());
                ray.move(ray_offset);

                const std::optional<T> bounding_distance = mesh.intersect_bounding(ray);
                if (bounding_distance)
                {
                        intersection = mesh.intersect(ray, *bounding_distance);
                        if (intersection.surface)
                        {
                                if (WITH_ERROR_LOG)
                                {
                                        LOG("The third intersection with ray #" + to_string(i) + "\n" + to_string(ray)
                                            + "\n" + "at point " + to_string(intersection.distance));
                                }
                                ++error_count;
                                continue;
                        }
                }
        }

        const double error_percent = 100.0 * error_count / rays.size();

        LOG("intersections " + to_string_fixed(duration_from(start_time), 5) + " s");
        LOG("");
        LOG(to_string(error_count) + " errors, " + to_string(rays.size()) + " rays, "
            + to_string_fixed(error_percent, 5) + "%");
        LOG("");

        if (!(error_percent < 0.005))
        {
                error("Too many errors, " + to_string_fixed(error_percent, 5) + "%");
        }
}

template <std::size_t N>
void create_spherical_mesh(
        const Vector<N, float>& center,
        const float radius,
        const int point_count,
        std::vector<Vector<N, float>>* const points,
        std::vector<std::array<int, N>>* const facets,
        ProgressRatio* const progress)
{
        std::mt19937_64 random_engine(point_count);
        points->resize(point_count);
        for (Vector<N, float>& p : *points)
        {
                p = radius * sampling::uniform_on_sphere<N, float>(random_engine) + center;
        }

        progress->set_text("Data: %v of %m");
        std::vector<geometry::ConvexHullFacet<N>> ch_facets;
        geometry::compute_convex_hull(*points, &ch_facets, progress, true);

        facets->clear();
        facets->reserve(ch_facets.size());
        for (const geometry::ConvexHullFacet<N>& ch_facet : ch_facets)
        {
                facets->push_back(ch_facet.vertices());
        }
}

template <std::size_t N, typename T>
float random_radius()
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        static constexpr std::array EXPONENTS =
                std::is_same_v<T, float>
                        ? std::to_array<std::array<int, 2>>({{-7, 10}, {-4, 6}, {-3, 5}, {-2, 3}})
                        : std::to_array<std::array<int, 2>>({{-22, 37}, {-22, 37}, {-22, 37}, {-22, 30}});

        static_assert(N >= 3 && N < 3 + EXPONENTS.size());
        static constexpr std::array<int, 2> MIN_MAX = EXPONENTS[N - 3];

        std::mt19937 random_engine = create_engine<std::mt19937>();
        const float exponent = std::uniform_real_distribution<float>(MIN_MAX[0], MIN_MAX[1])(random_engine);
        return std::pow(10.0f, exponent);
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<const Shape<N, T, Color>> create_spherical_mesh(const int point_count, ProgressRatio* const progress)
{
        LOG("painter random sphere");

        const float radius = random_radius<N, T>();
        const Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));
        LOG("point count " + to_string(point_count));

        LOG("spherical mesh in " + space_name(N) + "...");
        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;
        create_spherical_mesh(center, radius, point_count, &points, &facets, progress);

        LOG("facet count = " + to_string(facets.size()));
        LOG("mesh...");
        std::unique_ptr<const mesh::Mesh<N>> mesh(mesh::create_mesh_for_facets(points, facets));

        LOG("painter mesh...");
        mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");
        std::vector<const mesh::MeshObject<N>*> mesh_objects;
        mesh_objects.push_back(&mesh_object);
        std::unique_ptr<const Shape<N, T, Color>> painter_mesh = create_mesh<N, T, Color>(mesh_objects, progress);

        LOG("painter random sphere created");

        return painter_mesh;
}

template <std::size_t N, typename T>
void test_mesh(
        const int point_low,
        const int point_high,
        const int ray_low,
        const int ray_high,
        ProgressRatio* const progress)
{
        ASSERT(point_low <= point_high);
        ASSERT(ray_low <= ray_high);

        LOG("----------- " + space_name(N) + ", " + type_name<T>() + " -----------");

        const auto [point_count, ray_count] = [&]()
        {
                std::mt19937 random_engine = create_engine<std::mt19937>();
                return std::make_tuple(
                        std::uniform_int_distribution<int>(point_low, point_high)(random_engine),
                        std::uniform_int_distribution<int>(ray_low, ray_high)(random_engine));
        }();

        std::unique_ptr mesh = create_spherical_mesh<N, T, color::Spectrum>(point_count, progress);

        test_spherical_mesh(*mesh, ray_count, progress);
}

void test_mesh_3(ProgressRatio* const progress)
{
        test_mesh<3, float>(500, 1000, 90'000, 110'000, progress);
        test_mesh<3, double>(500, 1000, 90'000, 110'000, progress);
}
void test_mesh_4(ProgressRatio* const progress)
{
        test_mesh<4, float>(500, 1000, 90'000, 110'000, progress);
        test_mesh<4, double>(500, 1000, 90'000, 110'000, progress);
}
void test_mesh_5(ProgressRatio* const progress)
{
        test_mesh<5, float>(1000, 2000, 90'000, 110'000, progress);
        test_mesh<5, double>(1000, 2000, 90'000, 110'000, progress);
}
void test_mesh_6(ProgressRatio* const progress)
{
        test_mesh<6, float>(1000, 2000, 90'000, 110'000, progress);
        test_mesh<6, double>(1000, 2000, 90'000, 110'000, progress);
}

TEST_SMALL("Mesh Intersections, 3-Space", test_mesh_3)
TEST_SMALL("Mesh Intersections, 4-Space", test_mesh_4)
TEST_LARGE("Mesh Intersections, 5-Space", test_mesh_5)
TEST_LARGE("Mesh Intersections, 6-Space", test_mesh_6)
}
}
