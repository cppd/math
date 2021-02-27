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

#include "sphere_mesh.h"

#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/random/engine.h>
#include <src/geometry/core/convex_hull.h>
#include <src/model/mesh_utility.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <random>
#include <vector>

namespace ns::painter
{
namespace
{
template <std::size_t N>
void create_spherical_convex_hull(
        const Vector<N, float>& center,
        float radius,
        int point_count,
        std::vector<Vector<N, float>>* points,
        std::vector<std::array<int, N>>* facets,
        ProgressRatio* progress)
{
        std::mt19937_64 random_engine(point_count);
        points->resize(point_count);
        for (Vector<N, float>& p : *points)
        {
                p = radius * sampling::uniform_on_sphere<N, float>(random_engine) + center;
        }

        progress->set_text("Data: %v of %m");
        std::vector<geometry::ConvexHullFacet<N>> ch_facets;
        geometry::compute_convex_hull(*points, &ch_facets, progress);

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
        float exponent = std::uniform_real_distribution<float>(MIN_MAX[0], MIN_MAX[1])(random_engine);
        return std::pow(10.0f, exponent);
}
}

template <std::size_t N, typename T>
std::unique_ptr<const Mesh<N, T>> simplex_mesh_of_random_sphere(
        const Color& color,
        const Color::DataType& metalness,
        int point_count,
        ProgressRatio* progress)
{
        LOG("painter random sphere");

        const float radius = random_radius<N, T>();
        const Vector<N, float> center(-radius / 2);

        LOG("mesh radius = " + to_string(radius));
        LOG("mesh center = " + to_string(center));
        LOG("point count " + to_string(point_count));

        LOG("spherical convex hull in " + space_name(N) + "...");
        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;
        create_spherical_convex_hull(center, radius, point_count, &points, &facets, progress);

        LOG("facet count = " + to_string(facets.size()));
        LOG("mesh...");
        std::unique_ptr<const mesh::Mesh<N>> mesh(mesh::create_mesh_for_facets(points, facets));

        LOG("painter mesh...");
        mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");
        {
                mesh::Writing writing(&mesh_object);
                writing.set_color(color);
                writing.set_metalness(metalness);
        }
        std::vector<const mesh::MeshObject<N>*> mesh_objects;
        mesh_objects.push_back(&mesh_object);
        std::unique_ptr<const Mesh<N, T>> painter_mesh = std::make_unique<const Mesh<N, T>>(mesh_objects, progress);

        LOG("painter random sphere created");

        return painter_mesh;
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
