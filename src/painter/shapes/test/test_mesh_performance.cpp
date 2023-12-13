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

#include "spherical_mesh.h"

#include <src/color/color.h>
#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/dimensions.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ns::painter::shapes::test
{
namespace
{
template <std::size_t N, typename T>
constexpr std::optional<Vector<N, T>> EMPTY_GEOMETRIC_NORMAL;

constexpr std::size_t GROUP_SIZE = 0x1000;

template <bool ANY, std::size_t N, typename T, typename Color>
        requires (!ANY)
[[nodiscard]] bool scene_intersect(const Scene<N, T, Color>& scene, Ray<N, T> ray)
{
        const SurfaceIntersection surface_1 = scene.intersect(EMPTY_GEOMETRIC_NORMAL<N, T>, ray);
        if (!surface_1)
        {
                return false;
        }
        ray.set_org(surface_1.point());
        return static_cast<bool>(scene.intersect(surface_1.geometric_normal(), ray));
}

template <bool ANY, std::size_t N, typename T, typename Color>
        requires (ANY)
[[nodiscard]] bool scene_intersect(const Scene<N, T, Color>& scene, const Ray<N, T>& ray)
{
        return scene.intersect_any(EMPTY_GEOMETRIC_NORMAL<N, T>, ray, Limits<T>::infinity());
}

template <bool ANY, std::size_t N, typename T, typename Color>
void test(const test::SphericalMesh<N, T, Color>& mesh, const std::vector<Ray<N, T>>& rays)
{
        const auto f = [&]
        {
                for (std::size_t i = 0; i < rays.size();)
                {
                        MemoryArena::thread_local_instance().clear();

                        const std::size_t r = rays.size() - i;
                        const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                        for (; i < c; ++i)
                        {
                                do_not_optimize(scene_intersect<ANY>(*mesh.scene.scene, rays[i]));
                        }
                }
        };

        const long long start_ray_count = mesh.scene.scene->thread_ray_count();
        const Clock::time_point start_time = Clock::now();

        for (int i = 0; i < 10; ++i)
        {
                f();
        }

        const double duration = duration_from(start_time);
        const long long ray_count = mesh.scene.scene->thread_ray_count() - start_ray_count;

        std::string s;
        s += "Mesh intersections <" + space_name(N) + ", " + type_name<T>() + ">";
        if (ANY)
        {
                s += " any";
        }
        s += ": " + to_string_digit_groups(mesh.facet_count) + " facets";
        s += ", " + to_string_digit_groups(std::llround(ray_count / duration)) + " o/s";
        LOG(s);
}

struct Parameters final
{
        int point_count;
        int ray_count;
};

template <std::size_t N, typename T>
void test(const Parameters& parameters, progress::Ratio* const progress)
{
        using Color = color::Spectrum;

        PCG engine;

        const test::SphericalMesh<N, T, Color> mesh =
                test::create_spherical_mesh_scene<N, T, Color>(parameters.point_count, engine, progress);

        test<false>(mesh, test::create_spherical_mesh_center_rays(mesh.bounding_box, parameters.ray_count, engine));
        test<true>(mesh, test::create_spherical_mesh_center_rays(mesh.bounding_box, parameters.ray_count, engine));
}

template <std::size_t N>
void test(const Parameters& parameters, progress::Ratio* const progress)
{
        test<N, float>(parameters, progress);
        test<N, double>(parameters, progress);
}

template <std::size_t N>
Parameters parameters()
{
        static_assert(N >= 3);
        switch (N)
        {
        case 3:
                return {.point_count = 150'000, .ray_count = 50'000};
        case 4:
                return {.point_count = 40'000, .ray_count = 20'000};
        case 5:
                return {.point_count = 10'000, .ray_count = 5'000};
        default:
                return {.point_count = 2'000, .ray_count = 500};
        }
}

void test_performance(progress::Ratio* const progress)
{
        [progress]<std::size_t... I>(std::index_sequence<I...>&&)
        {
                (test<I>(parameters<I>(), progress), ...);
        }(settings::Dimensions());
}

TEST_PERFORMANCE("Mesh Intersections", test_performance)
}
}
