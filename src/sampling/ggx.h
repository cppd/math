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

/*
 Eric Heitz.
 Sampling the GGX Distribution of Visible Normals.
 Journal of Computer Graphics Techniques (JCGT), vol. 7, no. 4, 1–13, 2018.
*/

#pragma once

#include "sphere_uniform.h"

#include <src/com/interpolation.h>
#include <src/com/math.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

namespace ns::sampling
{
template <typename T, typename RandomEngine>
Vector<3, T> ggx_vn(RandomEngine& random_engine, const Vector<3, T>& ve, T alpha)
{
        // Section 3.2: transforming the view direction to the hemisphere configuration
        Vector<3, T> vh = Vector<3, T>(alpha * ve[0], alpha * ve[1], ve[2]).normalized();

        // Section 4.1: orthonormal basis (with special case if cross product is zero)
        Vector<3, T> t0 = [&]
        {
                T length_square = square(vh[0]) + square(vh[1]);
                if (length_square > 0)
                {
                        T length = std::sqrt(length_square);
                        return Vector<3, T>(-vh[1] / length, vh[0] / length, 0);
                }
                return Vector<3, T>(1, 0, 0);
        }();
        Vector<3, T> t1 = cross(vh, t0);

        // Section 4.2: parameterization of the projected area
        Vector<2, T> t = [&random_engine]
        {
                Vector<2, T> vector;
                T vector_length_square;
                uniform_in_sphere(random_engine, vector, vector_length_square);
                return vector;
        }();
        T s = T(0.5) * (T(1) + vh[2]);
        t[1] = interpolation(std::sqrt((1 - t[0]) * (1 + t[0])), t[1], s);

        // Section 4.3: reprojection onto hemisphere
        Vector<3, T> nh = [&]
        {
                T z = std::sqrt(std::max(T(0), 1 - dot(t, t)));
                return t[0] * t0 + t[1] * t1 + z * vh;
        }();

        // Section 3.4: transforming the normal back to the ellipsoid configuration
        Vector<3, T> ne = Vector<3, T>(alpha * nh[0], alpha * nh[1], std::max(T(0), nh[2])).normalized();

        return ne;
}

template <typename T, typename RandomEngine>
Vector<3, T> ggx_vn(RandomEngine& random_engine, const Vector<3, T>& normal, const Vector<3, T>& v, T alpha)
{
        std::array<Vector<3, T>, 2> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        Vector<3, T> ve;
        ve[0] = dot(v, basis[0]);
        ve[1] = dot(v, basis[1]);
        ve[2] = dot(v, normal);

        Vector<3, T> ne = ggx_vn(random_engine, ve, alpha);

        Vector<3, T> res = ne[2] * normal;
        for (unsigned i = 0; i < 2; ++i)
        {
                res += ne[i] * basis[i];
        }
        return res;
}
}
