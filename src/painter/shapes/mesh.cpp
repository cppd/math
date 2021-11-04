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

#include "mesh_facet.h"
#include "mesh_texture.h"

#include "../objects.h"

#include <src/color/color.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/object_tree.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>
#include <src/shading/ggx_diffuse.h>

#include <algorithm>
#include <optional>
#include <utility>

namespace ns::painter
{
namespace
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, const int offset, const bool add)
{
        std::array<int, N> r;
        if (add)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = offset + src[i];
                }
        }
        else
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = -1;
                }
        }
        return r;
}

template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, const int offset)
{
        std::array<int, N> r;
        for (unsigned i = 0; i < N; ++i)
        {
                r[i] = offset + src[i];
        }
        return r;
}

template <std::size_t N, typename T>
geometry::BoundingBox<N, T> compute_bounding_box(const std::vector<MeshFacet<N, T>>& facets)
{
        if (facets.empty())
        {
                error("No mesh facets");
        }
        geometry::BoundingBox<N, T> box = facets[0].bounding_box();
        for (std::size_t i = 1; i < facets.size(); ++i)
        {
                box.merge(facets[i].bounding_box());
        }
        return box;
}

template <typename T, typename Color>
struct Material
{
        T metalness;
        T roughness;
        Color color;
        T alpha;
        int image;

        Material(const T metalness, const T roughness, const color::Color& color, const int image, const T alpha)
                : metalness(std::clamp(metalness, T(0), T(1))),
                  roughness(std::clamp(roughness, T(0), T(1))),
                  color(color.to_color<Color>().clamp(0, 1)),
                  alpha(std::clamp(alpha, T(0), T(1))),
                  image(image)
        {
        }
};

template <std::size_t N, typename T, typename Color>
class Mesh final : public Shape<N, T, Color>
{
        std::vector<Vector<N, T>> vertices_;
        std::vector<Vector<N, T>> normals_;
        std::vector<Vector<N - 1, T>> texcoords_;
        std::vector<Material<T, Color>> materials_;
        std::vector<MeshTexture<N - 1>> images_;
        std::vector<MeshFacet<N, T>> facets_;

        geometry::BoundingBox<N, T> bounding_box_;
        std::optional<geometry::ObjectTree<MeshFacet<N, T>>> tree_;

        //

        void create(const mesh::Reading<N>& mesh_object);
        void create(const std::vector<mesh::Reading<N>>& mesh_objects);

        //

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override;

        ShapeIntersection<N, T, Color> intersect(const Ray<N, T>&, T bounding_distance) const override;

        geometry::BoundingBox<N, T> bounding_box() const override;

        std::function<bool(const geometry::ShapeIntersection<geometry::ParallelotopeAA<N, T>>&)> intersection_function()
                const override;

        //

public:
        Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* progress);

        const std::vector<Material<T, Color>>& materials() const
        {
                return materials_;
        }

        const std::vector<MeshTexture<N - 1>>& images() const
        {
                return images_;
        }

        // Mesh facets store pointers to vertices_, normals_, texcoords_
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;
};

template <std::size_t N, typename T, typename Color>
class SurfaceImpl final : public Surface<N, T, Color>
{
        const Mesh<N, T, Color>* mesh_;
        const MeshFacet<N, T>* facet_;

        Color surface_color(const Material<T, Color>& m) const
        {
                if (facet_->has_texcoord() && m.image >= 0)
                {
                        Vector<3, float> rgb = mesh_->images()[m.image].color(facet_->texcoord(this->point()));
                        return Color(rgb[0], rgb[1], rgb[2]);
                }
                return m.color;
        }

public:
        SurfaceImpl(const Vector<N, T>& point, const Mesh<N, T, Color>* mesh, const MeshFacet<N, T>* facet)
                : Surface<N, T, Color>(point), mesh_(mesh), facet_(facet)
        {
        }

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

                const Material<T, Color>& m = mesh_->materials()[facet_->material()];

                return shading::ggx_diffuse::f(m.metalness, m.roughness, surface_color(m), n, v, l);
        }

        T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                ASSERT(facet_->material() >= 0);

                const Material<T, Color>& m = mesh_->materials()[facet_->material()];

                return shading::ggx_diffuse::pdf(m.roughness, n, v, l);
        }

        Sample<N, T, Color> sample_brdf(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                ASSERT(facet_->material() >= 0);

                const Material<T, Color>& m = mesh_->materials()[facet_->material()];

                shading::Sample<N, T, Color> sample =
                        shading::ggx_diffuse::sample_f(random_engine, m.metalness, m.roughness, surface_color(m), n, v);

                Sample<N, T, Color> s;
                s.l = sample.l;
                s.pdf = sample.pdf;
                s.brdf = sample.brdf;
                s.specular = false;
                return s;
        }
};

template <std::size_t N, typename T, typename Color>
void Mesh<N, T, Color>::create(const mesh::Reading<N>& mesh_object)
{
        const T alpha = std::clamp<T>(mesh_object.alpha(), 0, 1);

        if (alpha == 0)
        {
                return;
        }

        const mesh::Mesh<N>& mesh = mesh_object.mesh();

        if (mesh.vertices.empty())
        {
                return;
        }
        if (mesh.facets.empty())
        {
                return;
        }

        int vertices_offset = vertices_.size();
        int normals_offset = normals_.size();
        int texcoords_offset = texcoords_.size();
        int materials_offset = materials_.size();
        int images_offset = images_.size();

        {
                const std::vector<Vector<N, T>>& vertices = to_vector<T>(mesh.vertices);
                vertices_.insert(vertices_.cend(), vertices.cbegin(), vertices.cend());
        }
        {
                auto iter_begin = std::next(vertices_.begin(), vertices_offset);
                auto iter_end = vertices_.end();
                std::transform(
                        iter_begin, iter_end, iter_begin,
                        matrix::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));
        }
        {
                const std::vector<Vector<N, T>>& normals = to_vector<T>(mesh.normals);
                normals_.insert(normals_.cend(), normals.cbegin(), normals.cend());
        }
        {
                const std::vector<Vector<N - 1, T>>& texcoords = to_vector<T>(mesh.texcoords);
                texcoords_.insert(texcoords_.cend(), texcoords.cbegin(), texcoords.cend());
        }

        bool facets_without_material = false;
        int default_material_index = mesh.materials.size();
        for (const typename mesh::Mesh<N>::Facet& facet : mesh.facets)
        {
                bool no_material = facet.material < 0;
                facets_without_material = facets_without_material || no_material;
                int facet_material = no_material ? default_material_index : facet.material;

                std::array<int, N> vertices = add_offset(facet.vertices, vertices_offset);
                std::array<int, N> normals = add_offset(facet.normals, normals_offset, facet.has_normal);
                std::array<int, N> texcoords = add_offset(facet.texcoords, texcoords_offset, facet.has_texcoord);
                int material = facet_material + materials_offset;

                facets_.emplace_back(
                        &vertices_, &normals_, &texcoords_, vertices, facet.has_normal, normals, facet.has_texcoord,
                        texcoords, material);
        }

        for (const typename mesh::Mesh<N>::Material& m : mesh.materials)
        {
                int image = m.image < 0 ? -1 : (images_offset + m.image);
                materials_.emplace_back(mesh_object.metalness(), mesh_object.roughness(), m.color, image, alpha);
        }
        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(materials_.size()));
                materials_.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1, alpha);
        }

        for (const image::Image<N - 1>& image : mesh.images)
        {
                images_.emplace_back(image);
        }
}

template <std::size_t N, typename T, typename Color>
void Mesh<N, T, Color>::create(const std::vector<mesh::Reading<N>>& mesh_objects)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        vertices_.clear();
        normals_.clear();
        texcoords_.clear();
        materials_.clear();
        images_.clear();
        facets_.clear();
        std::size_t vertex_count = 0;
        std::size_t normal_count = 0;
        std::size_t texcoord_count = 0;
        std::size_t material_count = 0;
        std::size_t image_count = 0;
        std::size_t facet_count = 0;
        for (const mesh::Reading<N>& mesh : mesh_objects)
        {
                vertex_count += mesh.mesh().vertices.size();
                normal_count += mesh.mesh().normals.size();
                texcoord_count += mesh.mesh().texcoords.size();
                bool no_material = false;
                for (const typename mesh::Mesh<N>::Facet& facet : mesh.mesh().facets)
                {
                        if (facet.material < 0)
                        {
                                no_material = true;
                                break;
                        }
                }
                material_count += mesh.mesh().materials.size() + (no_material ? 1 : 0);
                image_count += mesh.mesh().images.size();
                facet_count += mesh.mesh().facets.size();
        }
        vertices_.reserve(vertex_count);
        normals_.reserve(normal_count);
        texcoords_.reserve(texcoord_count);
        materials_.reserve(material_count);
        images_.reserve(image_count);
        facets_.reserve(facet_count);

        for (const mesh::Reading<N>& mesh_object : mesh_objects)
        {
                create(mesh_object);
        }

        ASSERT(vertex_count == vertices_.size());
        ASSERT(normal_count == normals_.size());
        ASSERT(texcoord_count == texcoords_.size());
        ASSERT(material_count == materials_.size());
        ASSERT(image_count == images_.size());
        ASSERT(facet_count == facets_.size());

        if (facets_.empty())
        {
                error("No facets found in meshes");
        }
}

template <std::size_t N, typename T, typename Color>
Mesh<N, T, Color>::Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        {
                std::vector<mesh::Reading<N>> reading;
                reading.reserve(mesh_objects.size());
                for (const mesh::MeshObject<N>* mesh_object : mesh_objects)
                {
                        reading.emplace_back(*mesh_object);
                }
                create(reading);
        }

        bounding_box_ = compute_bounding_box(facets_);

        progress->set_text(to_string(1 << N) + "-tree: %v of %m");

        tree_.emplace(&facets_, bounding_box_, TREE_MIN_OBJECTS_PER_BOX, progress);

        LOG("Painter mesh object created, " + to_string_fixed(duration_from(start_time), 5) + " s");
}

template <std::size_t N, typename T, typename Color>
std::optional<T> Mesh<N, T, Color>::intersect_bounding(const Ray<N, T>& r) const
{
        return tree_->intersect_root(r);
}

template <std::size_t N, typename T, typename Color>
ShapeIntersection<N, T, Color> Mesh<N, T, Color>::intersect(const Ray<N, T>& ray, const T bounding_distance) const
{
        std::optional<std::tuple<T, const MeshFacet<N, T>*>> v = tree_->intersect(ray, bounding_distance);
        if (!v)
        {
                return ShapeIntersection<N, T, Color>(nullptr);
        }
        const Vector<N, T> point = ray.point(std::get<0>(*v));
        ShapeIntersection<N, T, Color> intersection;
        intersection.distance = std::get<0>(*v);
        intersection.surface = make_arena_ptr<SurfaceImpl<N, T, Color>>(point, this, std::get<1>(*v));
        return intersection;
}

template <std::size_t N, typename T, typename Color>
geometry::BoundingBox<N, T> Mesh<N, T, Color>::bounding_box() const
{
        return bounding_box_;
}

template <std::size_t N, typename T, typename Color>
std::function<bool(const geometry::ShapeIntersection<geometry::ParallelotopeAA<N, T>>&)> Mesh<N, T, Color>::
        intersection_function() const
{
        auto root = std::make_shared<geometry::ParallelotopeAA<N, T>>(bounding_box_.min(), bounding_box_.max());
        return [root, w = geometry::ShapeIntersection(root.get())](
                       const geometry::ShapeIntersection<geometry::ParallelotopeAA<N, T>>& p)
        {
                return geometry::shape_intersection(w, p);
        };
}
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Shape<N, T, Color>> create_mesh(
        const std::vector<const mesh::MeshObject<N>*>& mesh_objects,
        ProgressRatio* const progress)
{
        return std::make_unique<Mesh<N, T, Color>>(mesh_objects, progress);
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
