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
#include <src/com/log.h>
#include <src/com/memory_arena.h>
#include <src/com/thread.h>
#include <src/com/time.h>
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
int tree_max_depth()
{
        static_assert(N >= 3);

        switch (N)
        {
        case 3:
                return 10;
        case 4:
                return 8;
        case 5:
                return 6;
        case 6:
                return 5;
        default:
                // Сумма геометрической прогрессии s = (pow(r, n) - 1) / (r - 1).
                // Для s и r найти n = log(s * (r - 1) + 1) / log(r).
                double s = 1e9;
                double r = std::pow(2, N);
                double n = std::log(s * (r - 1) + 1) / std::log(r);
                return std::max(2.0, std::floor(n));
        }
}

template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, int offset, bool add)
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
std::array<int, N> add_offset(const std::array<int, N>& src, int offset)
{
        std::array<int, N> r;
        for (unsigned i = 0; i < N; ++i)
        {
                r[i] = offset + src[i];
        }
        return r;
}

template <typename T>
struct Material
{
        T metalness;
        T roughness;
        Color Kd;
        int map_Kd;
        Color::DataType alpha;
        Material(T metalness, T roughness, const Color& Kd, int map_Kd, Color::DataType alpha)
                : metalness(std::clamp(metalness, T(0), T(1))),
                  roughness(std::clamp(roughness, T(0), T(1))),
                  Kd(Kd.clamped()),
                  map_Kd(map_Kd),
                  alpha(alpha)
        {
        }
};

template <std::size_t N, typename T>
class Mesh final : public Shape<N, T>
{
        std::vector<Vector<N, T>> m_vertices;
        std::vector<Vector<N, T>> m_normals;
        std::vector<Vector<N - 1, T>> m_texcoords;
        std::vector<Material<T>> m_materials;
        std::vector<MeshTexture<N - 1>> m_images;
        std::vector<MeshFacet<N, T>> m_facets;

        std::optional<geometry::ObjectTree<MeshFacet<N, T>>> m_tree;

        //

        void create(const mesh::Reading<N>& mesh_object);
        void create(const std::vector<mesh::Reading<N>>& mesh_objects);

        //

        std::optional<T> intersect_bounding(const Ray<N, T>& r) const override;

        const Surface<N, T>* intersect(const Ray<N, T>&, T bounding_distance) const override;

        geometry::BoundingBox<N, T> bounding_box() const override;

        std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>
                intersection_function() const override;

        //

public:
        Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* progress);

        const std::vector<Material<T>>& materials() const
        {
                return m_materials;
        }

        const std::vector<MeshTexture<N - 1>>& images() const
        {
                return m_images;
        }

        // Грани имеют адреса первых элементов векторов вершин,
        // нормалей и текстурных координат, поэтому нельзя копировать.
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) = delete;
};

template <std::size_t N, typename T>
class IntersectionImpl final : public Surface<N, T>
{
        const Mesh<N, T>* m_mesh;
        const MeshFacet<N, T>* m_facet;

public:
        IntersectionImpl(const Vector<N, T>& point, const Mesh<N, T>* mesh, const MeshFacet<N, T>* facet)
                : Surface<N, T>(point), m_mesh(mesh), m_facet(facet)
        {
        }

        Vector<N, T> geometric_normal() const override
        {
                return m_facet->geometric_normal();
        }

        std::optional<Vector<N, T>> shading_normal() const override
        {
                return m_facet->shading_normal(this->point());
        }

        std::optional<Color> light_source() const override
        {
                return std::nullopt;
        }

        Color brdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                ASSERT(m_facet->material() >= 0);

                const Material<T>& m = m_mesh->materials()[m_facet->material()];

                const Color color = [&]
                {
                        if (m_facet->has_texcoord() && m.map_Kd >= 0)
                        {
                                return m_mesh->images()[m.map_Kd].color(m_facet->texcoord(this->point()));
                        }
                        return m.Kd;
                }();

                return shading::GGXDiffuseBRDF<N, T>::f(m.metalness, m.roughness, color, n, v, l);
        }

        shading::Sample<N, T, Color> sample_brdf(
                RandomEngine<T>& random_engine,
                const Vector<N, T>& n,
                const Vector<N, T>& v) const override
        {
                ASSERT(m_facet->material() >= 0);

                const Material<T>& m = m_mesh->materials()[m_facet->material()];

                const Color color = [&]
                {
                        if (m_facet->has_texcoord() && m.map_Kd >= 0)
                        {
                                return m_mesh->images()[m.map_Kd].color(m_facet->texcoord(this->point()));
                        }
                        return m.Kd;
                }();

                return shading::GGXDiffuseBRDF<N, T>::sample_f(random_engine, m.metalness, m.roughness, color, n, v);
        }
};

template <std::size_t N, typename T>
void Mesh<N, T>::create(const mesh::Reading<N>& mesh_object)
{
        const Color::DataType alpha = std::clamp<Color::DataType>(mesh_object.alpha(), 0, 1);

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

        int vertices_offset = m_vertices.size();
        int normals_offset = m_normals.size();
        int texcoords_offset = m_texcoords.size();
        int materials_offset = m_materials.size();
        int images_offset = m_images.size();

        {
                const std::vector<Vector<N, T>>& vertices = to_vector<T>(mesh.vertices);
                m_vertices.insert(m_vertices.cend(), vertices.cbegin(), vertices.cend());
        }
        {
                auto iter_begin = std::next(m_vertices.begin(), vertices_offset);
                auto iter_end = m_vertices.end();
                std::transform(
                        iter_begin, iter_end, iter_begin,
                        matrix::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));
        }
        {
                const std::vector<Vector<N, T>>& normals = to_vector<T>(mesh.normals);
                m_normals.insert(m_normals.cend(), normals.cbegin(), normals.cend());
        }
        {
                const std::vector<Vector<N - 1, T>>& texcoords = to_vector<T>(mesh.texcoords);
                m_texcoords.insert(m_texcoords.cend(), texcoords.cbegin(), texcoords.cend());
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

                m_facets.emplace_back(
                        m_vertices, m_normals, m_texcoords, vertices, facet.has_normal, normals, facet.has_texcoord,
                        texcoords, material);
        }

        for (const typename mesh::Mesh<N>::Material& m : mesh.materials)
        {
                int map_Kd = m.map_Kd < 0 ? -1 : (images_offset + m.map_Kd);
                m_materials.emplace_back(mesh_object.metalness(), mesh_object.roughness(), m.Kd, map_Kd, alpha);
        }
        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(m_materials.size()));
                m_materials.emplace_back(
                        mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1, alpha);
        }

        for (const image::Image<N - 1>& image : mesh.images)
        {
                m_images.emplace_back(image);
        }
}

template <std::size_t N, typename T>
void Mesh<N, T>::create(const std::vector<mesh::Reading<N>>& mesh_objects)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        m_vertices.clear();
        m_normals.clear();
        m_texcoords.clear();
        m_materials.clear();
        m_images.clear();
        m_facets.clear();
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
        m_vertices.reserve(vertex_count);
        m_normals.reserve(normal_count);
        m_texcoords.reserve(texcoord_count);
        m_materials.reserve(material_count);
        m_images.reserve(image_count);
        m_facets.reserve(facet_count);

        for (const mesh::Reading<N>& mesh_object : mesh_objects)
        {
                create(mesh_object);
        }

        if (m_vertices.empty() || m_facets.empty())
        {
                error("No visible geometry found in meshes");
        }

        ASSERT(vertex_count == m_vertices.size());
        ASSERT(normal_count == m_normals.size());
        ASSERT(texcoord_count == m_texcoords.size());
        ASSERT(material_count == m_materials.size());
        ASSERT(image_count == m_images.size());
        ASSERT(facet_count == m_facets.size());
}

template <std::size_t N, typename T>
Mesh<N, T>::Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* progress)
{
        TimePoint start_time = time();

        {
                std::vector<mesh::Reading<N>> reading;
                reading.reserve(mesh_objects.size());
                for (const mesh::MeshObject<N>* mesh_object : mesh_objects)
                {
                        reading.emplace_back(*mesh_object);
                }
                create(reading);
        }

        progress->set_text(to_string(1 << N) + "-tree: %v of %m");

        m_tree.emplace(m_facets, tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, progress);

        LOG("Painter mesh object created, " + to_string_fixed(duration_from(start_time), 5) + " s");
}

template <std::size_t N, typename T>
std::optional<T> Mesh<N, T>::intersect_bounding(const Ray<N, T>& r) const
{
        return m_tree->intersect_root(r);
}

template <std::size_t N, typename T>
const Surface<N, T>* Mesh<N, T>::intersect(const Ray<N, T>& ray, const T bounding_distance) const
{
        std::optional<std::tuple<T, const MeshFacet<N, T>*>> v = m_tree->intersect(ray, bounding_distance);
        if (!v)
        {
                return nullptr;
        }
        return make_arena_ptr<IntersectionImpl<N, T>>(ray.point(std::get<0>(*v)), this, std::get<1>(*v));
}

template <std::size_t N, typename T>
geometry::BoundingBox<N, T> Mesh<N, T>::bounding_box() const
{
        return m_tree->bounding_box();
}

template <std::size_t N, typename T>
std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)> Mesh<N, T>::
        intersection_function() const
{
        return [w = geometry::ShapeWrapperForIntersection(m_tree->root())](
                       const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>& p)
        {
                return geometry::shape_intersection(w, p);
        };
}
}

template <std::size_t N, typename T>
std::unique_ptr<Shape<N, T>> create_mesh(
        const std::vector<const mesh::MeshObject<N>*>& mesh_objects,
        ProgressRatio* progress)
{
        return std::make_unique<Mesh<N, T>>(mesh_objects, progress);
}

#define CREATE_MESH_INSTANTIATION_N_T(N, T)                  \
        template std::unique_ptr<Shape<(N), T>> create_mesh( \
                const std::vector<const mesh::MeshObject<(N)>*>&, ProgressRatio*);

#define CREATE_MESH_INSTANTIATION_N(N)            \
        CREATE_MESH_INSTANTIATION_N_T((N), float) \
        CREATE_MESH_INSTANTIATION_N_T((N), double)

CREATE_MESH_INSTANTIATION_N(3)
CREATE_MESH_INSTANTIATION_N(4)
CREATE_MESH_INSTANTIATION_N(5)
CREATE_MESH_INSTANTIATION_N(6)
}
