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

#include <map>
#include <random>
#include <unordered_set>
#include <utility>

// Надо располагать точки по целым числам, так как выпуклая оболочка работает с целыми числами
// Для float большое число не надо
constexpr unsigned DISCRETIZATION = 100000;

constexpr double COS_FOR_BOUND = -0.3;

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
        const std::map<std::string, std::vector<Vector<N, float>> (ObjectRepository<N>::*)(unsigned) const> m_map = {
                {"Ellipsoid", &ObjectRepository<N>::ellipsoid},
                {"Ellipsoid, bound", &ObjectRepository<N>::ellipsoid_bound},
                {"Sphere with notch", &ObjectRepository<N>::sphere_with_notch},
                {"Sphere with notch, bound", &ObjectRepository<N>::sphere_with_notch_bound}};

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
