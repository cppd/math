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

#include "meshes.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>
#include <src/geometry/shapes/mobius_strip.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/shapes/torus.h>
#include <src/model/mesh_utility.h>
#include <src/sampling/sphere_uniform.h>
#include <src/settings/instantiation.h>

#include <functional>
#include <map>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace ns::storage
{
namespace
{
constexpr int POINT_DISCRETIZATION = 100000;

constexpr double LAST_AXIS_VALUE = -0.3;
constexpr double MOBIUS_STRIP_WIDTH = 1;

template <std::size_t N, typename T>
constexpr T last_axis(const Vector<N, T>& v)
{
        return v[N - 1];
}

template <std::size_t N>
class DiscretePoints
{
        std::vector<Vector<N, float>> points_;
        std::unordered_set<Vector<N, int>> integer_points_;

        template <typename T>
        static Vector<N, int> to_integer(const Vector<N, T>& v, const int factor)
        {
                static_assert(std::is_floating_point_v<T>);

                Vector<N, int> r;
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = std::lround(v[i] * factor);
                }
                return r;
        }

        template <typename T>
        static bool points_are_unique(const std::vector<T>& points)
        {
                return points.size() == std::unordered_set<T>(points.cbegin(), points.cend()).size();
        }

public:
        explicit DiscretePoints(const unsigned point_count)
        {
                points_.reserve(point_count);
                integer_points_.reserve(point_count);
        }

        template <typename T>
        void add(const Vector<N, T>& p)
        {
                Vector<N, int> integer_point = to_integer(p, POINT_DISCRETIZATION);
                if (!integer_points_.contains(integer_point))
                {
                        integer_points_.insert(integer_point);
                        points_.push_back(to_vector<float>(p));
                }
        }

        [[nodiscard]] unsigned size() const
        {
                return points_.size();
        }

        std::vector<Vector<N, float>> release()
        {
                ASSERT(integer_points_.size() == points_.size());
                ASSERT(points_are_unique(points_));

                integer_points_.clear();

                return std::move(points_);
        }
};

template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& engine, const bool bound)
{
        if (!bound)
        {
                return sampling::uniform_on_sphere<N, T>(engine);
        }
        Vector<N, T> v;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(engine);
        } while (last_axis(v) < LAST_AXIS_VALUE);
        return v;
}

#if 0
std::vector<Vector<2, float>> generate_points_semicircle(const unsigned point_count)
{
        if (point_count < 3)
        {
                error("point count out of range");
        }

        DiscretePoints<2> points(point_count);

        for (unsigned i = 0; i < point_count; ++i)
        {
                points.add(Vector<2, double>(-std::cos(PI<double> * i / (point_count - 1)),
                                             std::sin(PI<double> * i / (point_count - 1))));
        }

        if (points.size() != point_count)
        {
                error("Error semicircle point count: requested " + to_string(point_count) + ", generated " +
                      to_string(points.size()));
        }

        return points.release();
}
#endif

template <std::size_t N>
std::vector<Vector<N, float>> generate_points_ellipsoid(const unsigned point_count, const bool bound)
{
        PCG engine(point_count);

        DiscretePoints<N> points(point_count);

        while (points.size() < point_count)
        {
                Vector<N, double> v = random_on_sphere<N, double>(engine, bound);
                v[0] *= 2;
                points.add(v);
        }

        return points.release();
}

template <std::size_t N>
std::vector<Vector<N, float>> generate_points_sphere_with_notch(const unsigned point_count, const bool bound)
{
        PCG engine(point_count);

        DiscretePoints<N> points(point_count);

        while (points.size() < point_count)
        {
                Vector<N, double> v = random_on_sphere<N, double>(engine, bound);
                const double cos = last_axis(v);
                if (cos > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.5 * power<5>(cos));
                }
                points.add(v);
        }

        return points.release();
}

std::vector<Vector<3, float>> generate_points_mobius_strip(const unsigned point_count)
{
        PCG engine(point_count);

        DiscretePoints<3> points(point_count);

        while (points.size() < point_count)
        {
                Vector<3, double> v = geometry::mobius_strip_point<double>(MOBIUS_STRIP_WIDTH, engine);
                points.add(v);
        }

        return points.release();
}

template <std::size_t N>
std::vector<Vector<N, float>> generate_points_torus(const unsigned point_count, const bool bound)
{
        static_assert(N >= 3);

        PCG engine(point_count);

        DiscretePoints<N> points(point_count);

        while (points.size() < point_count)
        {
                const Vector<N, double> v = geometry::torus_point<N, double>(engine);
                if (bound && last_axis(v) < LAST_AXIS_VALUE)
                {
                        continue;
                }
                points.add(v);
        }

        return points.release();
}

//

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> ellipsoid(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_ellipsoid<N>(point_count, false));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> ellipsoid_bound(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_ellipsoid<N>(point_count, true));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> sphere_with_notch(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_sphere_with_notch<N>(point_count, false));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> sphere_with_notch_bound(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_sphere_with_notch<N>(point_count, true));
}

std::unique_ptr<model::mesh::Mesh<3>> mobius_strip(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_mobius_strip(point_count));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> torus(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_torus<N>(point_count, false));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> torus_bound(const unsigned point_count)
{
        return model::mesh::create_mesh_for_points(generate_points_torus<N>(point_count, true));
}

template <std::size_t N>
std::unique_ptr<model::mesh::Mesh<N>> sphere(const unsigned facet_count)
{
        std::vector<Vector<N, float>> points;
        std::vector<std::array<int, N>> facets;
        geometry::create_sphere(facet_count, &points, &facets);

        constexpr bool WRITE_LOG = true;
        return model::mesh::create_mesh_for_facets(points, facets, WRITE_LOG);
}

//

template <typename T>
std::vector<std::string> names_of_map(const std::map<std::string, T>& map)
{
        std::vector<std::string> names;
        names.reserve(map.size());

        for (const auto& e : map)
        {
                names.push_back(e.first);
        }

        return names;
}

template <std::size_t N>
class Impl final : public MeshObjectRepository<N>
{
        std::map<std::string, std::function<std::unique_ptr<model::mesh::Mesh<N>>(unsigned)>> map_point_;
        std::map<std::string, std::function<std::unique_ptr<model::mesh::Mesh<N>>(unsigned)>> map_facet_;

        [[nodiscard]] std::vector<std::string> point_object_names() const override
        {
                return names_of_map(map_point_);
        }

        [[nodiscard]] std::vector<std::string> facet_object_names() const override
        {
                return names_of_map(map_facet_);
        }

        [[nodiscard]] std::unique_ptr<model::mesh::Mesh<N>> point_object(
                const std::string& object_name,
                const unsigned point_count) const override
        {
                const auto iter = map_point_.find(object_name);
                if (iter != map_point_.cend())
                {
                        return iter->second(point_count);
                }
                error("Point object not found in repository: " + object_name);
        }

        [[nodiscard]] std::unique_ptr<model::mesh::Mesh<N>> facet_object(
                const std::string& object_name,
                const unsigned facet_count) const override
        {
                const auto iter = map_facet_.find(object_name);
                if (iter != map_facet_.cend())
                {
                        return iter->second(facet_count);
                }
                error("Facet object not found in repository: " + object_name);
        }

public:
        Impl()
        {
                map_point_.emplace("Ellipsoid", ellipsoid<N>);
                map_point_.emplace("Ellipsoid, bound", ellipsoid_bound<N>);

                map_point_.emplace("Sphere with a notch", sphere_with_notch<N>);
                map_point_.emplace("Sphere with a notch, bound", sphere_with_notch_bound<N>);

                if constexpr (N == 3)
                {
                        map_point_.emplace(reinterpret_cast<const char*>(u8"Möbius strip"), mobius_strip);
                }

                map_point_.emplace("Torus", torus<N>);
                map_point_.emplace("Torus, bound", torus_bound<N>);

                map_facet_.emplace("Sphere", sphere<N>);
        }
};
}

template <std::size_t N>
std::unique_ptr<MeshObjectRepository<N>> create_mesh_object_repository()
{
        return std::make_unique<Impl<N>>();
}

#define TEMPLATE(N) template std::unique_ptr<MeshObjectRepository<(N)>> create_mesh_object_repository<(N)>();

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
