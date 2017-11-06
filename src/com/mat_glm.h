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

#if 0

#include "mat.h"
#include "vec_glm.h"

#include <glm/gtc/matrix_access.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <type_traits>
#include <utility>

template <size_t N>
using glm_float_matrix =
        std::conditional_t<N == 2, glm::mat2, std::conditional_t<N == 3, glm::mat3, std::conditional_t<N == 4, glm::mat4, void>>>;

template <size_t N>
using glm_double_matrix =
        std::conditional_t<N == 2, glm::dmat2,
                           std::conditional_t<N == 3, glm::dmat3, std::conditional_t<N == 4, glm::dmat4, void>>>;

template <size_t N, typename T>
using glm_matrix = std::conditional_t<std::is_same_v<T, float>, glm_float_matrix<N>,
                                      std::conditional_t<std::is_same_v<T, double>, glm_double_matrix<N>, void>>;

namespace MatrixGLMImplementation
{
template <typename Dst, size_t N, typename Src, size_t... I>
glm_matrix<N, Dst> convert_to_glm(std::integer_sequence<size_t, I...>, const Matrix<N, N, Src>& m)
{
        return {to_glm<Dst>(m.column(I))...};
}

template <typename Dst, size_t N, typename Src, size_t... I>
Matrix<N, N, Dst> convert_to_matrix(std::integer_sequence<size_t, I...>, const glm_matrix<N, Src>& m)
{
        return {to_vector<Dst>(glm::row(m, I))...};
}

template <typename Dst, size_t N, typename Src>
Matrix<N, N, Dst> to_matrix(const glm_matrix<N, Src>& m)
{
        return convert_to_matrix<Dst, N, Src>(std::make_integer_sequence<size_t, N>(), m);
}
}

template <typename Dst, size_t N, typename Src>
glm_matrix<N, Dst> to_glm(const Matrix<N, N, Src>& m)
{
        return MatrixGLMImplementation::convert_to_glm<Dst, N, Src>(std::make_integer_sequence<size_t, N>(), m);
}

template <typename Dst>
Matrix<2, 2, Dst> to_matrix(const glm::mat2& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 2, float>(m);
}
template <typename Dst>
Matrix<3, 3, Dst> to_matrix(const glm::mat3& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 3, float>(m);
}
template <typename Dst>
Matrix<4, 4, Dst> to_matrix(const glm::mat4& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 4, float>(m);
}

template <typename Dst>
Matrix<2, 2, Dst> to_matrix(const glm::dmat2& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 2, double>(m);
}
template <typename Dst>
Matrix<3, 3, Dst> to_matrix(const glm::dmat3& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 3, double>(m);
}
template <typename Dst>
Matrix<4, 4, Dst> to_matrix(const glm::dmat4& m)
{
        return MatrixGLMImplementation::to_matrix<Dst, 4, double>(m);
}

#endif
