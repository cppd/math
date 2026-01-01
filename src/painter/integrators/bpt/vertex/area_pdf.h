/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/sampling/pdf.h>

#include <cmath>
#include <cstddef>

namespace ns::painter::integrators::bpt::vertex
{
template <std::size_t N, typename T, typename Normal>
[[nodiscard]] T solid_angle_pdf_to_area_pdf(
        const numerical::Vector<N, T>& prev_pos,
        const T angle_pdf,
        const numerical::Vector<N, T>& next_pos,
        const Normal& next_normal)
{
        const numerical::Vector<N, T> v = prev_pos - next_pos;
        const T distance = v.norm();
        const T cosine = [&]
        {
                if constexpr (requires { dot(v, *next_normal); })
                {
                        return next_normal ? (std::abs(dot(v, *next_normal)) / distance) : 1;
                }
                else
                {
                        return std::abs(dot(v, next_normal)) / distance;
                }
        }();
        return sampling::solid_angle_pdf_to_area_pdf<N, T>(angle_pdf, cosine, distance);
}

template <std::size_t N, typename T, typename Normal>
[[nodiscard]] T solid_angle_pdf_to_area_pdf(
        const T angle_pdf,
        const numerical::Vector<N, T>& next_dir,
        const T next_distance,
        const Normal& next_normal)
{
        ASSERT(next_dir.is_unit());
        const T cosine = [&]
        {
                if constexpr (requires { dot(next_dir, *next_normal); })
                {
                        return next_normal ? std::abs(dot(next_dir, *next_normal)) : 1;
                }
                else
                {
                        return std::abs(dot(next_dir, next_normal));
                }
        }();
        return sampling::solid_angle_pdf_to_area_pdf<N, T>(angle_pdf, cosine, next_distance);
}

template <std::size_t N, typename T, typename Normal>
[[nodiscard]] T pos_pdf_to_area_pdf(const T pos_pdf, const numerical::Vector<N, T>& dir, const Normal& next_normal)
{
        ASSERT(dir.is_unit());
        const T cosine = [&]
        {
                if constexpr (requires { dot(dir, *next_normal); })
                {
                        return next_normal ? std::abs(dot(dir, *next_normal)) : 1;
                }
                else
                {
                        return std::abs(dot(dir, next_normal));
                }
        }();
        return pos_pdf * cosine;
}
}
