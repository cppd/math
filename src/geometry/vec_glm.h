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

#pragma once

#include "vec.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

template <size_t N>
struct GLM_VEC;

template <>
struct GLM_VEC<2>
{
        typedef glm::vec2 t;
};
template <>
struct GLM_VEC<3>
{
        typedef glm::vec3 t;
};
template <>
struct GLM_VEC<4>
{
        typedef glm::vec4 t;
};

template <size_t N>
using glm_vec = typename GLM_VEC<N>::t;

template <size_t N, typename T>
glm_vec<N> to_glm(const Vector<N, T>& v)
{
        glm_vec<N> result;
        for (unsigned n = 0; n < N; ++n)
        {
                result[n] = v[n];
        }
        return result;
}

template <size_t N, typename T>
Vector<N, T> to_vector(const glm_vec<N>& v)
{
        Vector<N, T> result;
        for (unsigned n = 0; n < N; ++n)
        {
                result[n] = v[n];
        }
        return result;
}

template <typename T>
Vector<2, T> to_vector(const glm::vec2& v)
{
        return to_vec<2, T>(v);
}
template <typename T>
Vector<3, T> to_vector(const glm::vec3& v)
{
        return to_vec<3, T>(v);
}
template <typename T>
Vector<4, T> to_vector(const glm::vec4& v)
{
        return to_vec<4, T>(v);
}

template <size_t N, typename T>
std::vector<glm_vec<N>> to_glm(const std::vector<Vector<N, T>>& source_points)
{
        std::vector<glm_vec<N>> points(source_points.size());
        for (unsigned i = 0; i < source_points.size(); ++i)
        {
                points[i] = to_glm(source_points[i]);
        }
        return points;
}

template <size_t N, typename T>
std::vector<Vector<N, T>> to_vector(const std::vector<glm_vec<N>>& source_points)
{
        std::vector<Vector<N, T>> points(source_points.size());
        for (unsigned i = 0; i < source_points.size(); ++i)
        {
                points[i] = to_vector<N, T>(source_points[i]);
        }
        return points;
}

template <typename T>
std::vector<Vector<2, T>> to_vector(const std::vector<glm::vec2>& source_points)
{
        return to_vector<2, T>(source_points);
}
template <typename T>
std::vector<Vector<3, T>> to_vector(const std::vector<glm::vec3>& source_points)
{
        return to_vector<3, T>(source_points);
}
template <typename T>
std::vector<Vector<4, T>> to_vector(const std::vector<glm::vec4>& source_points)
{
        return to_vector<4, T>(source_points);
}
