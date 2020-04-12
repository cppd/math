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

#include "meshes.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/model/mesh_utility.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/random.h>

#include <functional>
#include <map>
#include <random>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace
{
// Надо располагать точки по целым числам, так как выпуклая оболочка работает с целыми числами.
// Для float большое число не надо.
constexpr unsigned DISCRETIZATION = 100000;

constexpr double COS_FOR_BOUND = -0.3;
constexpr double MOBIUS_STRIP_WIDTH = 1;

template <typename T, size_t... I, typename V>
constexpr Vector<sizeof...(I) + 1, T> make_last_axis(V&& value, std::integer_sequence<size_t, I...>&&)
{
        return {(static_cast<void>(I), 0)..., std::forward<V>(value)};
}

template <size_t N, typename T>
constexpr Vector<N, T> LAST_AXIS = make_last_axis<T>(1, std::make_integer_sequence<size_t, N - 1>());

#if 0
template <typename T, size_t... I, typename V>
constexpr Vector<sizeof...(I) + 1, T> add_axis(
        const Vector<sizeof...(I), T>& vector,
        V&& value,
        std::integer_sequence<size_t, I...>&&)
{
        return {vector[I]..., std::forward<V>(value)};
}

template <size_t N, typename T>
constexpr Vector<N + 1, T> add_dimension_with_zero(const Vector<N, T>& v)
{
        return add_axis(v, 0, std::make_integer_sequence<size_t, N>());
}

template <size_t N, typename T, typename V>
constexpr Vector<N, T> vector_with_last_dimension(V&& v)
{
        static_assert(N >= 2);

        return make_last_axis<T>(std::forward<V>(v), std::make_integer_sequence<size_t, N - 1>());
}
#endif

template <size_t N>
class DiscretePoints
{
        std::vector<Vector<N, float>> m_points;
        std::unordered_set<Vector<N, long>> m_integer_points;

        template <typename T>
        static Vector<N, long> to_integer(const Vector<N, T>& v, long factor)
        {
                static_assert(std::is_floating_point_v<T>);

                Vector<N, long> r;
                for (unsigned n = 0; n < N; ++n)
                {
                        r[n] = std::lround(v[n] * factor);
                }
                return r;
        }

        template <typename T>
        static bool points_are_unique(const std::vector<T>& points)
        {
                std::unordered_set<T> check_set(points.cbegin(), points.cend());
                return points.size() == check_set.size();
        }

public:
        explicit DiscretePoints(unsigned point_count)
        {
                m_points.reserve(point_count);
                m_integer_points.reserve(point_count);
        }

        template <typename T>
        void add(const Vector<N, T>& p)
        {
                Vector<N, long> integer_point = to_integer(p, DISCRETIZATION);
                if (m_integer_points.count(integer_point) == 0)
                {
                        m_integer_points.insert(integer_point);
                        m_points.push_back(to_vector<float>(p));
                }
        }

        unsigned size() const
        {
                return m_points.size();
        }

        std::vector<Vector<N, float>> release()
        {
                ASSERT(m_integer_points.size() == m_points.size());
                ASSERT(points_are_unique(m_points));

                m_integer_points.clear();

                std::vector<Vector<N, float>> points = std::move(m_points);

                return points;
        }
};

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_on_sphere(RandomEngine& engine)
{
        static_assert(std::is_floating_point_v<T>);
        std::uniform_real_distribution<T> urd(-1.0, 1.0);
        Vector<N, T> v;
        do
        {
                v = random_vector<N, T>(engine, urd);
        } while (dot(v, v) > 1);
        return v.normalized();
}

template <size_t N, typename T, typename V, typename RandomEngine>
Vector<N, T> random_on_sphere_bound(RandomEngine& engine, V cos_alpha)
{
        Vector<N, T> v;
        do
        {
                v = random_on_sphere<N, T>(engine);
        } while (dot(v, LAST_AXIS<N, T>) < cos_alpha);
        return v;
}

#if 0
std::vector<Vector<2, float>> generate_points_semicircle(unsigned point_count)
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

template <size_t N>
std::vector<Vector<N, float>> generate_points_ellipsoid(unsigned point_count, bool bound)
{
        DiscretePoints<N> points(point_count);

        std::mt19937_64 engine(point_count);

        while (points.size() < point_count)
        {
                Vector<N, double> v = (!bound) ? random_on_sphere<N, double>(engine)
                                               : random_on_sphere_bound<N, double>(engine, COS_FOR_BOUND);

                v[0] *= 2;

                points.add(v);
        }

        return points.release();
}

// Точки на сфере с углублением со стороны последней оси
// в положительном направлении этой оси
template <size_t N>
std::vector<Vector<N, float>> generate_points_sphere_with_notch(unsigned point_count, bool bound)
{
        DiscretePoints<N> points(point_count);

        std::mt19937_64 engine(point_count);

        while (points.size() < point_count)
        {
                Vector<N, double> v = (!bound) ? random_on_sphere<N, double>(engine)
                                               : random_on_sphere_bound<N, double>(engine, COS_FOR_BOUND);

                double dot_z = dot(LAST_AXIS<N, double>, v);
                if (dot_z > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.5 * std::pow(dot_z, 5));
                }

                points.add(v);
        }

        return points.release();
}

// На входе от 0 до 2 * PI, на выходе от 0 до PI
double mobius_curve(double x)
{
        x = x / (2 * PI<double>);

        x = 2 * x - 1;
        x = std::copysign(std::pow(std::abs(x), 5), x);
        x = (x + 1) / 2;

        return PI<double> * x;
}

std::vector<Vector<3, float>> generate_points_mobius_strip(unsigned point_count)
{
        DiscretePoints<3> points(point_count);

        std::mt19937_64 engine(point_count);

        std::uniform_real_distribution<double> urd_line(-MOBIUS_STRIP_WIDTH / 2, MOBIUS_STRIP_WIDTH / 2);
        std::uniform_real_distribution<double> urd_alpha(0, 2 * PI<double>);

        while (points.size() < point_count)
        {
                double alpha = urd_alpha(engine);

                // Случайная точка вдоль Z, вращение вокруг Y, смещение по X и вращение вокруг Z
                Vector<3, double> v(0, 0, urd_line(engine));
                v = rotate_vector(Vector<3, double>(0, 1, 0), PI<double> / 2 - mobius_curve(alpha), v);
                v += Vector<3, double>(1, 0, 0);
                v = rotate_vector(Vector<3, double>(0, 0, 1), alpha, v);

                points.add(v);
        }

        return points.release();
}

/*
 Sasho Kalajdzievski.
 An Illustrated Introduction to Topology and Homotopy.
 CRC Press, 2015.

 5.1 Finite Products of Spaces.
 14.4 Regarding Classification of CW-Complexes and Higher Dimensional Manifolds.
*/
template <size_t N, typename T, typename Engine>
Vector<N, T> torus_point(Engine& engine)
{
        static_assert(N >= 3);

        Vector<N, T> v(0);
        T v_length;
        Vector<N, T> sum(0);

        v[0] = 2;
        v_length = 2;

        for (size_t n = 1; n < N; ++n)
        {
                Vector<N, T> ortho(0);
                ortho[n] = v_length;

                Vector<2, T> s = random_on_sphere<2, T>(engine);
                Vector<N, T> vn = T(0.5) * (s[0] * v + s[1] * ortho);
                sum += vn;

                v = vn;
                v_length *= 0.5;
        }
        return sum;
}

// Точки на торе без равномерного распределения по его поверхности
template <size_t N>
std::vector<Vector<N, float>> generate_points_torus(unsigned point_count, bool bound)
{
        static_assert(N >= 3);

        DiscretePoints<N> points(point_count);

        std::mt19937_64 engine(point_count);

        while (points.size() < point_count)
        {
                Vector<N, double> v = torus_point<N, double>(engine);

                if (bound && dot(v, LAST_AXIS<N, double>) < COS_FOR_BOUND)
                {
                        continue;
                }

                points.add(v);
        }

        return points.release();
}

//

template <size_t N>
std::vector<Vector<N, float>> ellipsoid(unsigned point_count)
{
        return generate_points_ellipsoid<N>(point_count, false);
}

template <size_t N>
std::vector<Vector<N, float>> ellipsoid_bound(unsigned point_count)
{
        return generate_points_ellipsoid<N>(point_count, true);
}

template <size_t N>
std::vector<Vector<N, float>> sphere_with_notch(unsigned point_count)
{
        return generate_points_sphere_with_notch<N>(point_count, false);
}

template <size_t N>
std::vector<Vector<N, float>> sphere_with_notch_bound(unsigned point_count)
{
        return generate_points_sphere_with_notch<N>(point_count, true);
}

std::vector<Vector<3, float>> mobius_strip(unsigned point_count)
{
        return generate_points_mobius_strip(point_count);
}

template <size_t N>
std::vector<Vector<N, float>> torus(unsigned point_count)
{
        return generate_points_torus<N>(point_count, false);
}

template <size_t N>
std::vector<Vector<N, float>> torus_bound(unsigned point_count)
{
        return generate_points_torus<N>(point_count, true);
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

template <size_t N>
class Impl final : public MeshObjectRepository<N>
{
        std::map<std::string, std::function<std::vector<Vector<N, float>>(unsigned)>> m_map;

        std::vector<std::string> object_names() const override
        {
                return names_of_map(m_map);
        }

        std::unique_ptr<mesh::Mesh<N>> object(const std::string& object_name, unsigned point_count) const override
        {
                auto iter = m_map.find(object_name);
                if (iter != m_map.cend())
                {
                        return mesh::create_mesh_for_points(iter->second(point_count));
                }
                error("Object not found in repository: " + object_name);
        }

public:
        Impl()
        {
                m_map.emplace("Ellipsoid", ellipsoid<N>);
                m_map.emplace("Ellipsoid, bound", ellipsoid_bound<N>);

                m_map.emplace("Sphere with a notch", sphere_with_notch<N>);
                m_map.emplace("Sphere with a notch, bound", sphere_with_notch_bound<N>);

                if constexpr (N == 3)
                {
                        m_map.emplace(reinterpret_cast<const char*>(u8"Möbius strip"), mobius_strip);
                }

                if constexpr (N >= 3)
                {
                        m_map.emplace("Torus", torus<N>);
                        m_map.emplace("Torus, bound", torus_bound<N>);
                }
        }
};
}

template <size_t N>
std::unique_ptr<MeshObjectRepository<N>> create_mesh_object_repository()
{
        return std::make_unique<Impl<N>>();
}

template std::unique_ptr<MeshObjectRepository<3>> create_mesh_object_repository<3>();
template std::unique_ptr<MeshObjectRepository<4>> create_mesh_object_repository<4>();
template std::unique_ptr<MeshObjectRepository<5>> create_mesh_object_repository<5>();
