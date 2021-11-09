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
#include <src/geometry/accelerators/object_bvh.h>
#include <src/shading/ggx_diffuse.h>

namespace ns::painter
{
namespace
{
template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const MeshData<N, T, Color>* mesh_data_;
        const MeshFacet<N, T>* facet_;

        Color surface_color(const Material<T, Color>& m) const
        {
                if (facet_->has_texcoord() && m.image >= 0)
                {
                        Vector<3, float> rgb = mesh_data_->images()[m.image].color(facet_->texcoord(this->point()));
                        return Color(rgb[0], rgb[1], rgb[2]);
                }
                return m.color;
        }

        //

        Vector<N, T> geometric_normal() const override
        {
                return facet_->geometric_normal();
        }

        std::optional<Vector<N, T>> shading_normal() const override
        {
                return facet_->shading_normal(this->point());
        }

        std::optional<Color> light_source() const override
        {
                return std::nullopt;
        }

        Color brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const Material<T, Color>& m = mesh_data_->materials()[facet_->material()];

                return shading::ggx_diffuse::f(m.metalness, m.roughness, surface_color(m), n, v, l);
        }

        T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const Material<T, Color>& m = mesh_data_->materials()[facet_->material()];

                return shading::ggx_diffuse::pdf(m.roughness, n, v, l);
        }

        Sample<N, T, Color> sample_brdf(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                ASSERT(facet_->material() >= 0);

                const Material<T, Color>& m = mesh_data_->materials()[facet_->material()];

                shading::Sample<N, T, Color> sample =
                        shading::ggx_diffuse::sample_f(random_engine, m.metalness, m.roughness, surface_color(m), n, v);

                Sample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                s.specular = false;
                return s;
        }

public:
        SurfaceImpl(
                const Vector<N, T>& point,
                const MeshData<N, T, Color>* const mesh_data,
                const MeshFacet<N, T>* const facet)
                : Surface<N, T, Color>(point), mesh_data_(mesh_data), facet_(facet)
        {
        }
};

template <std::size_t N, typename T>
geometry::ObjectBvh<N, T, MeshFacet<N, T>> create_object_bvh(
        const std::vector<MeshFacet<N, T>>* const facets,
        ProgressRatio* const progress)
{
        progress->set_text("Painter mesh, " + space_name(N) + ", " + type_name<T>());
        progress->set(0);

        const Clock::time_point start_time = Clock::now();

        geometry::ObjectBvh<N, T, MeshFacet<N, T>> object_bvh(facets, progress);

        LOG("Painter mesh created, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return object_bvh;
}

template <std::size_t N, typename T, typename Color>
class ShapeImpl final : public Shape<N, T, Color>
{
        MeshData<N, T, Color> mesh_data_;
        std::optional<geometry::ObjectBvh<N, T, MeshFacet<N, T>>> object_bvh_;
        geometry::BoundingBox<N, T> bounding_box_;
        T intersection_cost_;

        T intersection_cost() const override
        {
                return intersection_cost_;
        }

        std::optional<T> intersect_bounding(const Ray<N, T>& ray, const T max_distance) const override
        {
                return object_bvh_->intersect_root(ray, max_distance);
        }

        ShapeIntersection<N, T, Color> intersect(
                const Ray<N, T>& ray,
                const T max_distance,
                const T /*bounding_distance*/) const override
        {
                std::optional<std::tuple<T, const MeshFacet<N, T>*>> v = object_bvh_->intersect(ray, max_distance);
                if (!v)
                {
                        return ShapeIntersection<N, T, Color>(nullptr);
                }
                const Vector<N, T> point = ray.point(std::get<0>(*v));
                ShapeIntersection<N, T, Color> intersection;
                intersection.distance = std::get<0>(*v);
                intersection.surface = make_arena_ptr<SurfaceImpl<N, T, Color>>(point, &mesh_data_, std::get<1>(*v));
                return intersection;
        }

        geometry::BoundingBox<N, T> bounding_box() const override
        {
                return bounding_box_;
        }

        std::function<bool(const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>&)> overlap_function()
                const override
        {
                auto root = std::make_shared<geometry::ParallelotopeAA<N, T>>(bounding_box_.min(), bounding_box_.max());
                return [root = root, overlap_function = root->overlap_function()](
                               const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>& p)
                {
                        return overlap_function(p);
                };
        }

public:
        ShapeImpl(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* const progress)
                : mesh_data_(mesh_objects),
                  object_bvh_(create_object_bvh(&mesh_data_.facets(), progress)),
                  bounding_box_(object_bvh_->bounding_box()),
                  intersection_cost_(mesh_data_.facets().size() * MeshFacet<N, T>::intersection_cost())
        {
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Shape<N, T, Color>> create_mesh(
        const std::vector<const mesh::MeshObject<N>*>& mesh_objects,
        ProgressRatio* const progress)
{
        return std::make_unique<ShapeImpl<N, T, Color>>(mesh_objects, progress);
}

#define CREATE_MESH_INSTANTIATION_N_T_C(N, T, C)                \
        template std::unique_ptr<Shape<(N), T, C>> create_mesh( \
                const std::vector<const mesh::MeshObject<(N)>*>&, ProgressRatio*);

#define CREATE_MESH_INSTANTIATION_N_T(N, T)                   \
        CREATE_MESH_INSTANTIATION_N_T_C((N), T, color::Color) \
        CREATE_MESH_INSTANTIATION_N_T_C((N), T, color::Spectrum)

#define CREATE_MESH_INSTANTIATION_N(N)            \
        CREATE_MESH_INSTANTIATION_N_T((N), float) \
        CREATE_MESH_INSTANTIATION_N_T((N), double)

CREATE_MESH_INSTANTIATION_N(3)
CREATE_MESH_INSTANTIATION_N(4)
CREATE_MESH_INSTANTIATION_N(5)
CREATE_MESH_INSTANTIATION_N(6)
}
