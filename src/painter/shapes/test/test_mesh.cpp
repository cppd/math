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

#include "../../objects.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/dimensions.h>
#include <src/test/test.h>

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

template <std::size_t N, typename T, typename Color>
bool intersections(const Scene<N, T, Color>& scene, Ray<N, T> ray)
{
        std::optional<Vector<N, T>> geometric_normal;

        {
                const SurfaceIntersection surface = scene.intersect(geometric_normal, ray);
                const bool intersection = scene.intersect_any(geometric_normal, ray, Limits<T>::infinity());

                if (!(static_cast<bool>(surface) == intersection))
                {
                        error("Intersect 1 is not equal to intersect any");
                }

                if (!surface)
                {
                        return false;
                }

                ray.set_org(surface.point());
                geometric_normal = surface.geometric_normal();
        }

        {
                const SurfaceIntersection surface = scene.intersect(geometric_normal, ray);
                const bool intersection = scene.intersect_any(geometric_normal, ray, Limits<T>::infinity());

                if (!(static_cast<bool>(surface) == intersection))
                {
                        error("Intersect 2 is not equal to intersect any");
                }

                if (!surface)
                {
                        return false;
                }

                ray.set_org(surface.point());
                geometric_normal = surface.geometric_normal();
        }

        const SurfaceIntersection surface = scene.intersect(geometric_normal, ray);
        const bool intersection = scene.intersect_any(geometric_normal, ray, Limits<T>::infinity());

        if (!(static_cast<bool>(surface) == intersection))
        {
                error("Intersect 3 is not equal to intersect any");
        }

        return !surface;
}

template <std::size_t N, typename T, typename Color>
void test_intersections(
        const test::SphericalMesh<N, T, Color>& mesh,
        const std::vector<Ray<N, T>>& rays,
        progress::Ratio* const progress)
{
        const double rays_size_reciprocal = 1.0 / rays.size();

        progress->set(0);
        progress->set_text(std::string("Ray intersections, ") + type_name<T>());

        int miss_count = 0;

        for (std::size_t i = 0; i < rays.size();)
        {
                MemoryArena::thread_local_instance().clear();

                const std::size_t r = rays.size() - i;
                const std::size_t c = (r >= GROUP_SIZE) ? i + GROUP_SIZE : rays.size();
                for (; i < c; ++i)
                {
                        if (!intersections(*mesh.scene.scene, rays[i]))
                        {
                                ++miss_count;
                        }
                }
                progress->set(i * rays_size_reciprocal);
        }

        std::string s;
        s += '<' + space_name(N) + ", " + type_name<T>() + '>';
        s += " miss count = " + to_string_digit_groups(miss_count);
        s += ", ray count = " + to_string_digit_groups(rays.size());
        if (!(miss_count <= std::lround(rays.size() * 2e-5)))
        {
                error("Too many intersection errors, " + s);
        }
        LOG(s);
}

template <std::size_t N, typename T, typename Color>
void test_surface_ratio(
        const test::SphericalMesh<N, T, Color>& mesh,
        const std::vector<Ray<N, T>>& rays,
        progress::Ratio* const progress)
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
                        if (mesh.scene.scene->intersect(EMPTY_GEOMETRIC_NORMAL<N, T>, rays[i]))
                        {
                                ++mesh_intersections;
                        }
                }
                progress->set(i * rays_size_reciprocal);
        }

        const T intersection_ratio = static_cast<T>(mesh_intersections) / rays.size();
        const T surface_ratio = mesh.surface / mesh.bounding_box.surface();

        const T relative_error =
                std::abs(intersection_ratio - surface_ratio)
                / std::max(std::abs(intersection_ratio), std::abs(surface_ratio));

        std::string s;
        s += '<' + space_name(N) + ", " + type_name<T>() + '>';
        s += " intersection ratio = " + to_string(intersection_ratio);
        s += ", area ratio = " + to_string(surface_ratio);
        if (!(relative_error < T{0.05}))
        {
                error("Intersection error, " + s + ", relative error " + to_string(relative_error));
        }
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

        const std::string name = "Test mesh intersections, " + space_name(N) + ", " + type_name<T>();

        LOG(name);

        PCG engine;

        const test::SphericalMesh<N, T, Color> mesh =
                test::create_spherical_mesh_scene<N, T, Color>(parameters.point_count, engine, progress);

        test_intersections(
                mesh, test::create_spherical_mesh_center_rays(mesh.bounding_box, parameters.ray_count, engine),
                progress);

        test_surface_ratio(
                mesh, test::create_random_intersections_rays(mesh.bounding_box, parameters.ray_count, engine),
                progress);

        LOG(name + " passed");
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
        case 4:
                return {.point_count = 1'000, .ray_count = 100'000};
        default:
                return {.point_count = 2'000, .ray_count = 100'000};
        }
}

template <std::size_t N>
void test_mesh(progress::Ratio* const progress)
{
        test<N>(parameters<N>(), progress);
}

template <std::size_t... I>
auto mesh_tests(std::index_sequence<I...>&&) noexcept
{
        return std::to_array({std::make_tuple(
                I <= 4 ? ns::test::Type::SMALL : ns::test::Type::LARGE,
                "Mesh Intersections, " + to_upper_first_letters(space_name(I)), test_mesh<I>)...});
}

TESTS(mesh_tests(settings::Dimensions()))
}
}
