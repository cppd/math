/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "sphere_mesh.h"

#include "../../sampling/sphere.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/geometry/core/convex_hull.h>
#include <src/model/mesh_utility.h>

#include <array>
#include <random>
#include <type_traits>
#include <vector>

namespace painter::shapes
{
namespace
{
template <typename T, typename RandomEngine, typename T1, typename T2>
T random_exponent(RandomEngine& random_engine, T1 exponent_low, T2 exponent_high)
{
        static_assert(std::is_floating_point_v<T>);
        ASSERT(exponent_low <= exponent_high);

        return std::pow(10, std::uniform_real_distribution<T>(exponent_low, exponent_high)(random_engine));
}

template <size_t N, typename T>
std::vector<Vector<N, T>> generate_random_points_on_sphere(const Vector<N, T>& center, T radius, int count)
{
        LOG("random points...");

        std::mt19937_64 random_engine(count);

        std::vector<Vector<N, T>> points(count);

        Vector<N, T> v;
        T length_square;

        for (int i = 0; i < count; ++i)
        {
                random_in_sphere(random_engine, v, length_square);
                v /= std::sqrt(length_square);
                points[i] = v * radius + center;
        }

        return points;
}

template <size_t N>
void create_spherical_convex_hull(
        const Vector<N, float>& center,
        float radius,
        int point_count,
        std::vector<Vector<N, float>>* points,
        std::vector<std::array<int, N>>* facets,
        ProgressRatio* progress)
{
        *points = generate_random_points_on_sphere<N, float>(center, radius, point_count);

        std::vector<geometry::ConvexHullFacet<N>> ch_facets;

        LOG("convex hull...");
        TimePoint start_time = time();
        geometry::compute_convex_hull(*points, &ch_facets, progress);
        LOG("convex hull created, " + to_string_fixed(duration_from(start_time), 5) + " s");
        LOG("facet count = " + to_string(ch_facets.size()));

        facets->clear();
        facets->reserve(ch_facets.size());
        for (const geometry::ConvexHullFacet<N>& ch_facet : ch_facets)
        {
                facets->push_back(ch_facet.vertices());
        }
}

template <size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> simplex_mesh_of_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        const Vector<N, float>& center,
        float radius,
        int point_count,
        ProgressRatio* progress)
{
        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;

        LOG("convex hull in " + space_name(N) + ", point count " + to_string(point_count));

        progress->set_text("Data: %v of %m");

        create_spherical_convex_hull(center, radius, point_count, &points, &facets, progress);

        LOG("mesh...");

        std::unique_ptr<const mesh::Mesh<N>> mesh(mesh::create_mesh_for_facets(points, facets));

        LOG("painter mesh...");

        mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");
        {
                mesh::Writing writing(&mesh_object);
                writing.set_color(color);
                writing.set_diffuse(diffuse);
        }
        std::vector<const mesh::MeshObject<N>*> meshes;
        meshes.push_back(&mesh_object);
        return std::make_unique<const Mesh<N, T>>(meshes, progress);
}
}

template <size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress)
{
        static_assert(N >= 3 && N <= 6);
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        // От 3 измерений последовательно
        constexpr std::array<std::array<int, 2>, 4> exponents_for_float = {{{-7, 10}, {-4, 6}, {-3, 5}, {-2, 3}}};
        constexpr std::array<std::array<int, 2>, 4> exponents_for_double = {
                {{-22, 37}, {-22, 37}, {-22, 37}, {-22, 30}}};

        const std::array<int, 2>* exponents = nullptr;
        if (std::is_same_v<T, float>)
        {
                exponents = &exponents_for_float[N - 3];
        }
        if (std::is_same_v<T, double>)
        {
                exponents = &exponents_for_double[N - 3];
        }
        ASSERT(exponents);

        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        float radius = random_exponent<float>(random_engine, (*exponents)[0], (*exponents)[1]);

        Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));

        return simplex_mesh_of_sphere<N, T>(color, diffuse, center, radius, point_count, progress);
}

template std::unique_ptr<const Mesh<3, float>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<4, float>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<5, float>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<6, float>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);

template std::unique_ptr<const Mesh<3, double>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<4, double>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<5, double>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
template std::unique_ptr<const Mesh<6, double>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& diffuse,
        int point_count,
        ProgressRatio* progress);
}
