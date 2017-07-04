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
#include <type_traits>
#include <utility>

namespace VectorGLMImplementation
{
template <size_t N>
using glm_vector =
        std::conditional_t<N == 2, glm::vec2, std::conditional_t<N == 3, glm::vec3, std::conditional_t<N == 4, glm::vec4, void>>>;

template <typename Dst, typename Src, size_t... I>
Dst convert_vector(std::integer_sequence<size_t, I...>, const Src& v)
{
        return {(v[I])...};
}

template <size_t N, typename T>
glm_vector<N> to_glm(const Vector<N, T>& v)
{
        return convert_vector<glm_vector<N>, Vector<N, T>>(std::make_integer_sequence<size_t, N>(), v);
}

template <size_t N, typename T>
Vector<N, T> to_vector(const glm_vector<N>& v)
{
        return convert_vector<Vector<N, T>, glm_vector<N>>(std::make_integer_sequence<size_t, N>(), v);
}

template <size_t N, typename T>
std::vector<glm_vector<N>> to_glm(const std::vector<Vector<N, T>>& v)
{
        std::vector<glm_vector<N>> points(v.size());
        for (unsigned i = 0; i < v.size(); ++i)
        {
                points[i] = to_glm(v[i]);
        }
        return points;
}

template <size_t N, typename T>
std::vector<Vector<N, T>> to_vector(const std::vector<glm_vector<N>>& v)
{
        std::vector<Vector<N, T>> points(v.size());
        for (unsigned i = 0; i < v.size(); ++i)
        {
                points[i] = to_vector<N, T>(v[i]);
        }
        return points;
}
}

template <typename T>
glm::vec2 to_glm(const Vector<2, T>& v)
{
        return VectorGLMImplementation::to_glm<2, T>(v);
}

template <typename T>
glm::vec3 to_glm(const Vector<3, T>& v)
{
        return VectorGLMImplementation::to_glm<3, T>(v);
}

template <typename T>
glm::vec4 to_glm(const Vector<4, T>& v)
{
        return VectorGLMImplementation::to_glm<4, T>(v);
}

//

template <typename T>
Vector<2, T> to_vector(const glm::vec2& v)
{
        return VectorGLMImplementation::to_vector<2, T>(v);
}

template <typename T>
Vector<3, T> to_vector(const glm::vec3& v)
{
        return VectorGLMImplementation::to_vector<3, T>(v);
}

template <typename T>
Vector<4, T> to_vector(const glm::vec4& v)
{
        return VectorGLMImplementation::to_vector<4, T>(v);
}

//

template <typename T>
std::vector<glm::vec2> to_glm(const std::vector<Vector<2, T>>& points)
{
        return VectorGLMImplementation::to_glm(points);
}

template <typename T>
std::vector<glm::vec3> to_glm(const std::vector<Vector<3, T>>& points)
{
        return VectorGLMImplementation::to_glm(points);
}

template <typename T>
std::vector<glm::vec4> to_glm(const std::vector<Vector<4, T>>& points)
{
        return VectorGLMImplementation::to_glm(points);
}

//

template <typename T>
std::vector<Vector<2, T>> to_vector(const std::vector<glm::vec2>& points)
{
        return VectorGLMImplementation::to_vector<2, T>(points);
}

template <typename T>
std::vector<Vector<3, T>> to_vector(const std::vector<glm::vec3>& points)
{
        return VectorGLMImplementation::to_vector<3, T>(points);
}

template <typename T>
std::vector<Vector<4, T>> to_vector(const std::vector<glm::vec4>& points)
{
        return VectorGLMImplementation::to_vector<4, T>(points);
}
