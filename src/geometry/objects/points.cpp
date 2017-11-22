/*
Copyright (C) 2017 Topological Manifold

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

#include "points.h"

#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/quaternion.h"

#include <map>
#include <random>
#include <unordered_set>
#include <utility>

// Надо располагать точки по целым числам, так как выпуклая оболочка работает с целыми числами
// Для float большое число не надо
constexpr unsigned DISCRETIZATION = 100000;

constexpr double COS_FOR_BOUND = -0.3;

constexpr double MOBIUS_STRIP_WIDTH = 1;

template <typename T, size_t... I>
constexpr Vector<sizeof...(I) + 1, T> make_z_axis(std::integer_sequence<size_t, I...>)
{
        return {(static_cast<void>(I), 0)..., 1};
}
template <size_t N, typename T>
constexpr Vector<N, T> Z_AXIS = make_z_axis<T>(std::make_integer_sequence<size_t, N - 1>());

namespace
{
template <size_t N>
void check_unique_points(const std::vector<Vector<N, float>>& points)
{
        std::unordered_set<Vector<N, float>> check_set(points.cbegin(), points.cend());

        if (points.size() != check_set.size())
        {
                error("error generate unique points");
        }
}

template <size_t N>
Vector<N, long> to_integer(const Vector<N, double>& v, long factor)
{
        Vector<N, long> r;
        for (unsigned n = 0; n < N; ++n)
        {
                r[n] = std::lround(v[n] * factor);
        }
        return r;
}

template <size_t N>
vec<N> random_sphere(std::mt19937_64* gen)
{
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        vec<N> v;
        do
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        v[n] = urd(*gen);
                }
        } while (dot(v, v) > 1);

        return normalize(v);
}

template <size_t N>
vec<N> random_sphere_bound(std::mt19937_64* gen, double cos_alpha)
{
        vec<N> v;
        do
        {
                v = random_sphere<N>(gen);
        } while (dot(v, Z_AXIS<N, double>) < cos_alpha);
        return v;
}

#if 0
std::vector<Vector<2, float>> generate_points_semicircle(unsigned point_count)
{
        if (point_count < 3)
        {
                error("point count out of range");
        }

        std::vector<Vector<2, float>> points(point_count);

        for (double i = 0; i < point_count; ++i)
        {
                points[i] = {-std::cos(PI * i / (point_count - 1)), std::sin(PI * i / (point_count - 1))};
        }

        check_unique_points(points);

        return points;
}
#endif

template <size_t N>
std::vector<Vector<N, float>> generate_points_ellipsoid(unsigned point_count, bool bound)
{
        std::vector<Vector<N, float>> points;
        points.reserve(point_count);

        std::unordered_set<Vector<N, long>> integer_points;
        integer_points.reserve(point_count);

        std::mt19937_64 gen(point_count);

        while (integer_points.size() < point_count)
        {
                vec<N> v = (!bound) ? random_sphere<N>(&gen) : random_sphere_bound<N>(&gen, COS_FOR_BOUND);

                v[0] *= 2;

                Vector<N, long> integer_point = to_integer(v, DISCRETIZATION);
                if (integer_points.count(integer_point) == 0)
                {
                        integer_points.insert(integer_point);
                        points.push_back(to_vector<float>(v));
                }
        }

        check_unique_points(points);

        return points;
}

template <size_t N>
std::vector<Vector<N, float>> generate_points_sphere_with_notch(unsigned point_count, bool bound)
{
        std::vector<Vector<N, float>> points;
        points.reserve(point_count);

        std::unordered_set<Vector<N, long>> integer_points;
        integer_points.reserve(point_count);

        std::mt19937_64 gen(point_count);

        while (integer_points.size() < point_count)
        {
                // точки на сфере с углублением со стороны последней оси
                // в положительном направлении этой оси

                vec<N> v = (!bound) ? random_sphere<N>(&gen) : random_sphere_bound<N>(&gen, COS_FOR_BOUND);

                double dot_z = dot(Z_AXIS<N, double>, v);
                if (dot_z > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.5 * std::pow(dot_z, 5));
                }

                Vector<N, long> integer_point = to_integer(v, DISCRETIZATION);
                if (integer_points.count(integer_point) == 0)
                {
                        integer_points.insert(integer_point);
                        points.push_back(to_vector<float>(v));
                }
        }

        check_unique_points(points);

        return points;
}

// На входе от 0 до 2 * PI, на выходе от 0 до PI.
double mobius_curve(double x)
{
        x = x / (2 * PI);

        x = 2 * x - 1;
        x = std::copysign(std::pow(std::abs(x), 5), x);
        x = (x + 1) / 2;

        return PI * x;
}

std::vector<Vector<3, float>> generate_points_mobius_strip(unsigned point_count)
{
        std::vector<Vector<3, float>> points;
        points.reserve(point_count);

        std::unordered_set<Vector<3, long>> integer_points;
        integer_points.reserve(point_count);

        std::mt19937_64 gen(point_count);

        std::uniform_real_distribution<double> urd_line(-MOBIUS_STRIP_WIDTH / 2, MOBIUS_STRIP_WIDTH / 2);
        std::uniform_real_distribution<double> urd_alpha(0, 2 * PI);

        while (integer_points.size() < point_count)
        {
                double alpha = urd_alpha(gen);

                // Случайная точка вдоль Z, вращение вокруг Y, смещение по X и вращение вокруг Z
                vec<3> v(0, 0, urd_line(gen));
                v = rotate_vector(vec<3>(0, 1, 0), PI / 2 - mobius_curve(alpha), v);
                v += vec<3>(1, 0, 0);
                v = rotate_vector(vec<3>(0, 0, 1), alpha, v);

                Vector<3, long> integer_point = to_integer(v, DISCRETIZATION);
                if (integer_points.count(integer_point) == 0)
                {
                        integer_points.insert(integer_point);
                        points.push_back(to_vector<float>(v));
                }
        }

        check_unique_points(points);

        return points;
}

template <typename T>
std::vector<std::string> get_names_of_map(const std::map<std::string, T>& m)
{
        std::vector<std::string> name_list;
        name_list.reserve(m.size());
        for (auto e : m)
        {
                name_list.push_back(e.first);
        }
        return name_list;
}

template <size_t N>
class ObjectRepository final : public IObjectRepository<N>
{
        std::map<std::string, std::vector<Vector<N, float>> (ObjectRepository<N>::*)(unsigned) const> m_map;

        std::vector<Vector<N, float>> ellipsoid(unsigned point_count) const override
        {
                return generate_points_ellipsoid<N>(point_count, false);
        }
        std::vector<Vector<N, float>> ellipsoid_bound(unsigned point_count) const override
        {
                return generate_points_ellipsoid<N>(point_count, true);
        }
        std::vector<Vector<N, float>> sphere_with_notch(unsigned point_count) const override
        {
                return generate_points_sphere_with_notch<N>(point_count, false);
        }
        std::vector<Vector<N, float>> sphere_with_notch_bound(unsigned point_count) const override
        {
                return generate_points_sphere_with_notch<N>(point_count, true);
        }
        std::vector<Vector<3, float>> mobius_strip(unsigned point_count) const
        {
                return generate_points_mobius_strip(point_count);
        }

        std::vector<std::string> get_list_of_point_objects() const override
        {
                return get_names_of_map(m_map);
        }
        std::vector<Vector<N, float>> get_point_object(const std::string& object_name, unsigned point_count) const override
        {
                auto iter = m_map.find(object_name);
                if (iter != m_map.cend())
                {
                        return (this->*(iter->second))(point_count);
                }
                error("object not found in repository: " + object_name);
        }

public:
        ObjectRepository()
        {
                m_map.emplace("Ellipsoid", &ObjectRepository<N>::ellipsoid);
                m_map.emplace("Ellipsoid, bound", &ObjectRepository<N>::ellipsoid_bound);
                m_map.emplace("Sphere with a notch", &ObjectRepository<N>::sphere_with_notch);
                m_map.emplace("Sphere with a notch, bound", &ObjectRepository<N>::sphere_with_notch_bound);
                if constexpr (N == 3)
                {
                        m_map.emplace(u8"Möbius strip", &ObjectRepository<N>::mobius_strip);
                }
        }
};
}

template <size_t N>
std::unique_ptr<IObjectRepository<N>> create_object_repository()
{
        return std::make_unique<ObjectRepository<N>>();
}

template std::unique_ptr<IObjectRepository<2>> create_object_repository<2>();
template std::unique_ptr<IObjectRepository<3>> create_object_repository<3>();
template std::unique_ptr<IObjectRepository<4>> create_object_repository<4>();
