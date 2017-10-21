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

template <size_t N>
using glm_float_vector =
        std::conditional_t<N == 2, glm::vec2, std::conditional_t<N == 3, glm::vec3, std::conditional_t<N == 4, glm::vec4, void>>>;

template <size_t N>
using glm_double_vector =
        std::conditional_t<N == 2, glm::dvec2,
                           std::conditional_t<N == 3, glm::dvec3, std::conditional_t<N == 4, glm::dvec4, void>>>;

template <size_t N, typename T>
using glm_vector = std::conditional_t<std::is_same_v<float, T>, glm_float_vector<N>,
                                      std::conditional_t<std::is_same_v<double, T>, glm_double_vector<N>, void>>;

namespace VectorGLMImplementation
{
template <typename Dst, typename Src, size_t... I>
Dst convert_vector(std::integer_sequence<size_t, I...>, const Src& v)
{
        return {(v[I])...};
}

template <size_t N, typename Dst, typename Src>
Vector<N, Dst> to_vector(const glm_vector<N, Src>& v)
{
        return convert_vector<Vector<N, Dst>>(std::make_integer_sequence<size_t, N>(), v);
}

template <size_t N, typename Dst, typename Src>
std::vector<Vector<N, Dst>> to_vector(const std::vector<glm_vector<N, Src>>& v)
{
        std::vector<Vector<N, Dst>> points(v.size());
        for (unsigned i = 0; i < v.size(); ++i)
        {
                points[i] = to_vector<N, Dst, Src>(v[i]);
        }
        return points;
}
}

template <typename Dst, size_t N, typename Src>
glm_vector<N, Dst> to_glm(const Vector<N, Src>& v)
{
        return VectorGLMImplementation::convert_vector<glm_vector<N, Dst>>(std::make_integer_sequence<size_t, N>(), v);
}

template <typename Dst, size_t N, typename Src>
std::vector<glm_vector<N, Dst>> to_glm(const std::vector<Vector<N, Src>>& v)
{
        std::vector<glm_vector<N, Dst>> points(v.size());
        for (unsigned i = 0; i < v.size(); ++i)
        {
                points[i] = to_glm<Dst, N, Src>(v[i]);
        }
        return points;
}

template <typename Dst>
Vector<2, Dst> to_vector(const glm::vec2& v)
{
        return VectorGLMImplementation::to_vector<2, Dst, float>(v);
}
template <typename Dst>
Vector<3, Dst> to_vector(const glm::vec3& v)
{
        return VectorGLMImplementation::to_vector<3, Dst, float>(v);
}
template <typename Dst>
Vector<4, Dst> to_vector(const glm::vec4& v)
{
        return VectorGLMImplementation::to_vector<4, Dst, float>(v);
}

template <typename Dst>
Vector<2, Dst> to_vector(const glm::dvec2& v)
{
        return VectorGLMImplementation::to_vector<2, Dst, double>(v);
}
template <typename Dst>
Vector<3, Dst> to_vector(const glm::dvec3& v)
{
        return VectorGLMImplementation::to_vector<3, Dst, double>(v);
}
template <typename Dst>
Vector<4, Dst> to_vector(const glm::dvec4& v)
{
        return VectorGLMImplementation::to_vector<4, Dst, double>(v);
}

template <typename Dst>
std::vector<Vector<2, Dst>> to_vector(const std::vector<glm::vec2>& v)
{
        return VectorGLMImplementation::to_vector<2, Dst, float>(v);
}
template <typename Dst>
std::vector<Vector<3, Dst>> to_vector(const std::vector<glm::vec3>& v)
{
        return VectorGLMImplementation::to_vector<3, Dst, float>(v);
}
template <typename Dst>
std::vector<Vector<4, Dst>> to_vector(const std::vector<glm::vec4>& v)
{
        return VectorGLMImplementation::to_vector<4, Dst, float>(v);
}

template <typename Dst>
std::vector<Vector<2, Dst>> to_vector(const std::vector<glm::dvec2>& v)
{
        return VectorGLMImplementation::to_vector<2, Dst, double>(v);
}
template <typename Dst>
std::vector<Vector<3, Dst>> to_vector(const std::vector<glm::dvec3>& v)
{
        return VectorGLMImplementation::to_vector<3, Dst, double>(v);
}
template <typename Dst>
std::vector<Vector<4, Dst>> to_vector(const std::vector<glm::dvec4>& v)
{
        return VectorGLMImplementation::to_vector<4, Dst, double>(v);
}
