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

#pragma once

#include "../../scenes/storage_scene.h"
#include "../mesh.h"

#include <src/geometry/core/convex_hull.h>
#include <src/geometry/shapes/simplex_volume.h>
#include <src/model/mesh_utility.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cmath>
#include <random>

namespace ns::painter::test
{
namespace spherical_mesh_implementation
{
inline constexpr bool WRITE_LOG = false;

template <std::size_t N, typename T>
Vector<N, T> random_center(const T radius, std::mt19937_64& random_engine)
{
        static_assert(N >= 3);

        const T v = radius / std::sqrt(static_cast<T>(N));

        std::bernoulli_distribution bd(0.5);
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = bd(random_engine) ? v : -v;
        }
        return res;
}

template <std::size_t N>
std::unique_ptr<const mesh::Mesh<N>> create_spherical_mesh(
        const float radius,
        const int point_count,
        std::mt19937_64& random_engine,
        ProgressRatio* const progress)
{
        const Vector<N, float> center = random_center<N>(radius, random_engine);

        std::vector<Vector<N, float>> points;
        points.resize(point_count);
        for (Vector<N, float>& p : points)
        {
                p = radius * sampling::uniform_on_sphere<N, float>(random_engine) + center;
        }

        progress->set_text("Data: %v of %m");
        std::vector<geometry::ConvexHullFacet<N>> ch_facets;
        geometry::compute_convex_hull(points, &ch_facets, progress, WRITE_LOG);

        std::vector<std::array<int, N>> facets;
        facets.reserve(ch_facets.size());
        for (const geometry::ConvexHullFacet<N>& ch_facet : ch_facets)
        {
                facets.push_back(ch_facet.vertices());
        }

        progress->set_text("Mesh");
        progress->set(0);
        return mesh::create_mesh_for_facets(points, facets, WRITE_LOG);
}

template <std::size_t N, typename T>
float random_radius(std::mt19937_64& random_engine)
{
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        static constexpr std::array EXPONENTS =
                std::is_same_v<T, float>
                        ? std::to_array<std::array<int, 2>>({{-7, 10}, {-4, 6}, {-3, 5}, {-2, 3}})
                        : std::to_array<std::array<int, 2>>({{-22, 37}, {-22, 37}, {-22, 37}, {-22, 30}});

        static_assert(N >= 3 && N < 3 + EXPONENTS.size());
        static constexpr std::array<int, 2> EXPONENT = EXPONENTS[N - 3];

        const float exponent = std::uniform_real_distribution<float>(EXPONENT[0], EXPONENT[1])(random_engine);
        return std::pow(10.0f, exponent);
}
}

template <std::size_t N, typename T, typename Color>
struct SphericalMesh
{
        std::size_t facet_count;
        geometry::BoundingBox<N, T> bounding_box;
        std::unique_ptr<const Scene<N, T, Color>> scene;
        T surface;
};

template <std::size_t N, typename T, typename Color>
SphericalMesh<N, T, Color> create_spherical_mesh_scene(
        const int point_count,
        std::mt19937_64& random_engine,
        ProgressRatio* const progress)
{
        namespace impl = spherical_mesh_implementation;

        SphericalMesh<N, T, Color> res;

        std::unique_ptr<const mesh::Mesh<N>> mesh = impl::create_spherical_mesh<N>(
                impl::random_radius<N, T>(random_engine), point_count, random_engine, progress);

        res.facet_count = mesh->facets.size();

        res.surface = 0;
        for (const typename mesh::Mesh<N>::Facet& facet : mesh->facets)
        {
                std::array<Vector<N, T>, N> vertices;
                for (std::size_t i = 0; i < N; ++i)
                {
                        vertices[i] = to_vector<T>(mesh->vertices[facet.vertices[i]]);
                }
                res.surface += geometry::simplex_volume(vertices);
        }

        mesh::MeshObject<N> mesh_object(std::move(mesh), Matrix<N + 1, N + 1, double>(1), "");

        std::vector<const mesh::MeshObject<N>*> mesh_objects;
        mesh_objects.push_back(&mesh_object);

        std::unique_ptr<const Shape<N, T, Color>> painter_mesh =
                create_mesh<N, T, Color>(mesh_objects, impl::WRITE_LOG, progress);

        res.bounding_box = painter_mesh->bounding_box();

        std::vector<std::unique_ptr<const Shape<N, T, Color>>> meshes;
        meshes.push_back(std::move(painter_mesh));

        res.scene = create_storage_scene(Color(), {}, {}, std::move(meshes), progress);

        return res;
}

template <std::size_t N, typename T>
std::vector<Ray<N, T>> create_spherical_mesh_center_rays(
        const geometry::BoundingBox<N, T>& bb,
        const int ray_count,
        std::mt19937_64& random_engine)
{
        const Vector<N, T> center = bb.center();
        const T radius = bb.diagonal().norm() / 2;

        std::vector<Ray<N, T>> rays;
        rays.resize(ray_count);
        for (Ray<N, T>& ray : rays)
        {
                Vector<N, T> v = sampling::uniform_on_sphere<N, T>(random_engine);
                ray = Ray<N, T>(radius * v + center, -v);
        }
        return rays;
}

template <std::size_t N, typename T, typename RandomEngine>
std::vector<Ray<N, T>> create_random_intersections_rays(
        const geometry::BoundingBox<N, T>& bb,
        const int ray_count,
        RandomEngine& engine)
{
        const Vector<N, T> diagonal = bb.diagonal();

        std::uniform_real_distribution<T> urd(-1, 2);
        const auto random_cover_point = [&]()
        {
                Vector<N, T> v = bb.min();
                for (std::size_t i = 0; i < N; ++i)
                {
                        v[i] += diagonal[i] * urd(engine);
                }
                return v;
        };

        std::vector<Ray<N, T>> rays;
        rays.reserve(ray_count);
        for (int i = 0; i < ray_count; ++i)
        {
                Ray<N, T> ray;
                do
                {
                        ray = Ray<N, T>(random_cover_point(), sampling::uniform_on_sphere<N, T>(engine));
                } while (!bb.intersect(ray));
                rays.push_back(ray);
        }
        return rays;
}
}