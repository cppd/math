/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "mesh/data.h"
#include "mesh/facet.h"
#include "mesh/material.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/accelerators/bvh_object.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/ray_intersection.h>
#include <src/geometry/spatial/shape_overlap.h>
#include <src/model/mesh_object.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>
#include <src/shading/ggx/brdf.h>
#include <src/shading/ggx/metalness.h>
#include <src/shading/objects.h>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ns::painter::shapes
{
namespace
{
template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const mesh::Mesh<N, T, Color>* mesh_;
        const mesh::Facet<N, T>* facet_;

        [[nodiscard]] shading::Colors<Color> surface_color(
                const numerical::Vector<N, T>& point,
                const mesh::Material<T, Color>& material) const
        {
                if (facet_->has_texcoord() && material.image() >= 0)
                {
                        const numerical::Vector<3, float> rgb =
                                mesh_->images[material.image()].color(facet_->texcoord(mesh_->texcoords, point));
                        const Color color = Color(rgb[0], rgb[1], rgb[2]);
                        return shading::ggx::compute_metalness(color, material.metalness());
                }
                return shading::ggx::compute_metalness(material.color(), material.metalness());
        }

        //

        [[nodiscard]] numerical::Vector<N, T> point(const numerical::Ray<N, T>& ray, const T distance) const override
        {
                return facet_->project(ray.point(distance));
        }

        [[nodiscard]] numerical::Vector<N, T> geometric_normal(const numerical::Vector<N, T>& /*point*/) const override
        {
                return facet_->geometric_normal();
        }

        [[nodiscard]] std::optional<numerical::Vector<N, T>> shading_normal(
                const numerical::Vector<N, T>& point) const override
        {
                return facet_->shading_normal(mesh_->normals, point);
        }

        [[nodiscard]] const LightSource<N, T, Color>* light_source() const override
        {
                return nullptr;
        }

        [[nodiscard]] Color brdf(
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const mesh::Material<T, Color>& material = mesh_->materials[facet_->material()];

                return shading::ggx::brdf::f(material.roughness(), surface_color(point, material), n, v, l);
        }

        [[nodiscard]] T pdf(
                const numerical::Vector<N, T>& /*point*/,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const mesh::Material<T, Color>& material = mesh_->materials[facet_->material()];

                return shading::ggx::brdf::pdf(material.roughness(), n, v, l);
        }

        [[nodiscard]] SurfaceSample<N, T, Color> sample(
                PCG& engine,
                const numerical::Vector<N, T>& point,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v) const override
        {
                ASSERT(facet_->material() >= 0);

                const mesh::Material<T, Color>& material = mesh_->materials[facet_->material()];

                const shading::Sample<N, T, Color>& sample = shading::ggx::brdf::sample_f(
                        engine, material.roughness(), surface_color(point, material), n, v);

                SurfaceSample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                return s;
        }

        [[nodiscard]] bool is_specular(const numerical::Vector<N, T>& /*point*/) const override
        {
                return false;
        }

        [[nodiscard]] T alpha(const numerical::Vector<N, T>& /*point*/) const override
        {
                ASSERT(facet_->material() >= 0);

                const mesh::Material<T, Color>& material = mesh_->materials[facet_->material()];

                return material.alpha();
        }

public:
        SurfaceImpl(const mesh::Mesh<N, T, Color>* const mesh, const mesh::Facet<N, T>* const facet)
                : mesh_(mesh),
                  facet_(facet)
        {
        }
};

template <std::size_t N, typename T, typename Color>
[[nodiscard]] std::vector<geometry::accelerators::BvhObject<N, T>> bvh_objects(
        const mesh::Mesh<N, T, Color>& mesh,
        const std::vector<std::array<int, N>>& facet_vertex_indices)
{
        const std::vector<mesh::Facet<N, T>>& facets = mesh.facets;
        std::vector<geometry::accelerators::BvhObject<N, T>> res;
        res.reserve(facets.size());
        for (std::size_t i = 0; i < facets.size(); ++i)
        {
                ASSERT(i < facet_vertex_indices.size());
                res.emplace_back(
                        geometry::spatial::BoundingBox(mesh.vertices, facet_vertex_indices[i]),
                        facets[i].intersection_cost(), i);
        }
        return res;
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] geometry::accelerators::Bvh<N, T> create_bvh(
        const mesh::Mesh<N, T, Color>& mesh,
        const std::vector<std::array<int, N>>& facet_vertex_indices,
        const bool write_log,
        progress::Ratio* const progress)
{
        progress->set_text("Painter mesh, " + space_name(N) + ", " + type_name<T>());
        progress->set(0);

        const Clock::time_point start_time = Clock::now();

        geometry::accelerators::Bvh<N, T> bvh(bvh_objects(mesh, facet_vertex_indices), progress);

        if (write_log)
        {
                LOG("Painter mesh created, " + to_string_fixed(duration_from(start_time), 5) + " s");
        }

        return bvh;
}

template <std::size_t N, typename T, typename Color>
class Impl final : public Shape<N, T, Color>
{
        mesh::Mesh<N, T, Color> mesh_;
        geometry::accelerators::Bvh<N, T> bvh_;
        geometry::spatial::BoundingBox<N, T> bounding_box_;
        T intersection_cost_;

        [[nodiscard]] T intersection_cost() const override
        {
                return intersection_cost_;
        }

        [[nodiscard]] std::optional<T> intersect_bounds(const numerical::Ray<N, T>& ray, const T max_distance)
                const override
        {
                return bvh_.intersect_root(ray, max_distance);
        }

        [[nodiscard]] ShapeIntersection<N, T, Color> intersect(
                const numerical::Ray<N, T>& ray,
                const T max_distance,
                const T /*bounding_distance*/) const override
        {
                const auto intersection = bvh_.intersect(
                        ray, max_distance,
                        [facets = &mesh_.facets, &ray](const auto& indices, const auto& max)
                                -> std::optional<std::tuple<T, const mesh::Facet<N, T>*>>
                        {
                                const std::tuple<T, const mesh::Facet<N, T>*> info =
                                        geometry::spatial::ray_intersection(*facets, indices, ray, max);
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
                return {distance, make_arena_ptr<SurfaceImpl<N, T, Color>>(&mesh_, facet)};
        }

        [[nodiscard]] bool intersect_any(
                const numerical::Ray<N, T>& ray,
                const T max_distance,
                const T /*bounding_distance*/) const override
        {
                return bvh_.intersect(
                        ray, max_distance,
                        [facets = &mesh_.facets, &ray](const auto& indices, const auto& max) -> bool
                        {
                                return geometry::spatial::ray_intersection_any(*facets, indices, ray, max);
                        });
        }

        [[nodiscard]] geometry::spatial::BoundingBox<N, T> bounding_box() const override
        {
                return bounding_box_;
        }

        [[nodiscard]] std::function<
                bool(const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>&)>
                overlap_function() const override
        {
                auto root = std::make_shared<geometry::spatial::ParallelotopeAA<N, T>>(
                        bounding_box_.min(), bounding_box_.max());
                return [root = root, overlap_function = root->overlap_function()](
                               const geometry::spatial::ShapeOverlap<geometry::spatial::ParallelotopeAA<N, T>>& p)
                {
                        return overlap_function(p);
                };
        }

        Impl(mesh::MeshData<N, T, Color>&& mesh_data, const bool write_log, progress::Ratio* const progress)
                : mesh_(std::move(mesh_data.mesh)),
                  bvh_(create_bvh(mesh_, mesh_data.facet_vertex_indices, write_log, progress)),
                  bounding_box_(bvh_.bounding_box()),
                  intersection_cost_(
                          mesh_.facets.size()
                          * std::remove_reference_t<decltype(mesh_.facets.front())>::intersection_cost())
        {
        }

public:
        Impl(const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
             const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
             const bool write_log,
             progress::Ratio* const progress)
                : Impl(mesh::create_mesh_data<N, T, Color>(mesh_objects, clip_plane_equation, write_log),
                       write_log,
                       progress)
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Shape<N, T, Color>> create_mesh(
        const std::vector<const model::mesh::MeshObject<N>*>& mesh_objects,
        const std::optional<numerical::Vector<N + 1, T>>& clip_plane_equation,
        const bool write_log,
        progress::Ratio* const progress)
{
        return std::make_unique<Impl<N, T, Color>>(mesh_objects, clip_plane_equation, write_log, progress);
}

#define TEMPLATE(N, T, C)                                                \
        template std::unique_ptr<Shape<(N), T, C>> create_mesh(          \
                const std::vector<const model::mesh::MeshObject<(N)>*>&, \
                const std::optional<numerical::Vector<(N) + 1, T>>&, bool, progress::Ratio*);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
