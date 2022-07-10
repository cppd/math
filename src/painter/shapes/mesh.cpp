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

#include "mesh.h"

#include "mesh_data.h"

#include "../objects.h"

#include <src/color/color.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/ray_intersection.h>
#include <src/settings/instantiation.h>
#include <src/shading/ggx_diffuse.h>
#include <src/shading/metalness.h>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const MeshData<N, T, Color>* mesh_data_;
        const MeshFacet<N, T>* facet_;

        [[nodiscard]] shading::Colors<Color> surface_color(
                const Vector<N, T>& point,
                const MeshMaterial<T, Color>& material) const
        {
                if (facet_->has_texcoord() && material.image() >= 0)
                {
                        const Vector<3, float> rgb = mesh_data_->images[material.image()].color(
                                facet_->texcoord(mesh_data_->texcoords, point));
                        const Color color = Color(rgb[0], rgb[1], rgb[2]);
                        return shading::compute_metalness(color, material.metalness());
                }
                return shading::compute_metalness(material.color(), material.metalness());
        }

        //

        [[nodiscard]] Vector<N, T> point(const Ray<N, T>& ray, const T& distance) const override
        {
                return facet_->project(ray.point(distance));
        }

        [[nodiscard]] Vector<N, T> geometric_normal(const Vector<N, T>& /*point*/) const override
        {
                return facet_->geometric_normal();
        }

        [[nodiscard]] std::optional<Vector<N, T>> shading_normal(const Vector<N, T>& point) const override
        {
                return facet_->shading_normal(mesh_data_->normals, point);
        }

        [[nodiscard]] std::optional<Color> light_source() const override
        {
                return std::nullopt;
        }

        [[nodiscard]] Color brdf(
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const MeshMaterial<T, Color>& material = mesh_data_->materials[facet_->material()];

                return shading::ggx_diffuse::f(material.roughness(), surface_color(point, material), n, v, l);
        }

        [[nodiscard]] T pdf(
                const Vector<N, T>& /*point*/,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const MeshMaterial<T, Color>& material = mesh_data_->materials[facet_->material()];

                return shading::ggx_diffuse::pdf(material.roughness(), n, v, l);
        }

        [[nodiscard]] Sample<N, T, Color> sample_brdf(
                PCG& engine,
                const Vector<N, T>& point,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override
        {
                ASSERT(facet_->material() >= 0);

                const MeshMaterial<T, Color>& material = mesh_data_->materials[facet_->material()];

                const shading::Sample<N, T, Color>& sample = shading::ggx_diffuse::sample_f(
                        engine, material.roughness(), surface_color(point, material), n, v);

                Sample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                s.specular = false;
                return s;
        }

public:
        SurfaceImpl(const MeshData<N, T, Color>* const mesh_data, const MeshFacet<N, T>* const facet)
                : mesh_data_(mesh_data),
                  facet_(facet)
        {
        }
};

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::vector<geometry::BvhObject<N, T>> bvh_objects(
        const MeshData<N, T, Color>& mesh_data,
        const std::vector<std::array<int, N>>& facet_vertex_indices)
{
        const std::vector<MeshFacet<N, T>>& facets = mesh_data.facets;
        std::vector<geometry::BvhObject<N, T>> res;
        res.reserve(facets.size());
        for (std::size_t i = 0; i < facets.size(); ++i)
        {
                ASSERT(i < facet_vertex_indices.size());
                res.emplace_back(
                        geometry::BoundingBox(mesh_data.vertices, facet_vertex_indices[i]),
                        facets[i].intersection_cost(), i);
        }
        return res;
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] geometry::Bvh<N, T> create_bvh(
        const MeshData<N, T, Color>& mesh_data,
        const std::vector<std::array<int, N>>& facet_vertex_indices,
        const bool write_log,
        progress::Ratio* const progress)
{
        progress->set_text("Painter mesh, " + space_name(N) + ", " + type_name<T>());
        progress->set(0);

        const Clock::time_point start_time = Clock::now();

        geometry::Bvh<N, T> bvh(bvh_objects(mesh_data, facet_vertex_indices), progress);

        if (write_log)
        {
                LOG("Painter mesh created, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }

        return bvh;
}

template <std::size_t N, typename T, typename Color>
class ShapeImpl final : public Shape<N, T, Color>
{
        MeshData<N, T, Color> mesh_data_;
        geometry::Bvh<N, T> bvh_;
        geometry::BoundingBox<N, T> bounding_box_;
        T intersection_cost_;

        [[nodiscard]] T intersection_cost() const override
        {
                return intersection_cost_;
        }

        [[nodiscard]] std::optional<T> intersect_bounds(const Ray<N, T>& ray, const T max_distance) const override
        {
                return bvh_.intersect_root(ray, max_distance);
        }

        [[nodiscard]] std::tuple<T, const Surface<N, T, Color>*> intersect(
                const Ray<N, T>& ray,
                const T max_distance,
                const T /*bounding_distance*/) const override
        {
                const auto intersection = bvh_.intersect(
                        ray, max_distance,
                        [facets = &mesh_data_.facets, &ray](const auto& indices, const auto& max)
                                -> std::optional<std::tuple<T, const MeshFacet<N, T>*>>
                        {
                                const std::tuple<T, const MeshFacet<N, T>*> info =
                                        geometry::ray_intersection(*facets, indices, ray, max);
                                if (std::get<1>(info))
                                {
                                        return info;
                                }
                                return std::nullopt;
                        });
                if (!intersection)
                {
                        return {0, nullptr};
                }
                const auto& [distance, facet] = *intersection;
                return {distance, make_arena_ptr<SurfaceImpl<N, T, Color>>(&mesh_data_, facet)};
        }

        [[nodiscard]] bool intersect_any(const Ray<N, T>& ray, const T max_distance, const T /*bounding_distance*/)
                const override
        {
                return bvh_.intersect(
                        ray, max_distance,
                        [facets = &mesh_data_.facets, &ray](const auto& indices, const auto& max) -> bool
                        {
                                return geometry::ray_intersection_any(*facets, indices, ray, max);
                        });
        }

        [[nodiscard]] geometry::BoundingBox<N, T> bounding_box() const override
        {
                return bounding_box_;
        }

        [[nodiscard]] std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)>
                overlap_function() const override
        {
                auto root = std::make_shared<geometry::ParallelotopeAA<N, T>>(bounding_box_.min(), bounding_box_.max());
                return [root = root, overlap_function = root->overlap_function()](
                               const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>& p)
                {
                        return overlap_function(p);
                };
        }

        ShapeImpl(MeshInfo<N, T, Color>&& mesh_info, const bool write_log, progress::Ratio* const progress)
                : mesh_data_(std::move(mesh_info.mesh_data)),
                  bvh_(create_bvh(mesh_data_, mesh_info.facet_vertex_indices, write_log, progress)),
                  bounding_box_(bvh_.bounding_box()),
                  intersection_cost_(
                          mesh_data_.facets.size()
                          * std::remove_reference_t<decltype(mesh_data_.facets.front())>::intersection_cost())
        {
        }

public:
        ShapeImpl(
                const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
                const std::optional<Vector<N + 1, T>>& clip_plane_equation,
                const bool write_log,
                progress::Ratio* const progress)
                : ShapeImpl(
                        create_mesh_info<N, T, Color>(mesh_objects, clip_plane_equation, write_log),
                        write_log,
                        progress)
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Shape<N, T, Color>> create_mesh(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<Vector<N + 1, T>>& clip_plane_equation,
        const bool write_log,
        progress::Ratio* const progress)
{
        return std::make_unique<ShapeImpl<N, T, Color>>(mesh_objects, clip_plane_equation, write_log, progress);
}

#define TEMPLATE(N, T, C)                                                                                          \
        template std::unique_ptr<Shape<(N), T, C>> create_mesh(                                                    \
                const std::vector<const model::mesh::MeshObject<(N)>*>&, const std::optional<Vector<(N) + 1, T>>&, \
                bool, progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
