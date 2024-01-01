/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "area_pdf.h"

#include "../../../objects.h"
#include "../../com/normals.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::painter::integrators::bpt::vertex
{
template <std::size_t N, typename T, typename Color>
class Surface final
{
        SurfaceIntersection<N, T, Color> surface_;
        Normals<N, T> normals_;
        Color beta_;
        Vector<N, T> dir_to_prev_;
        T pdf_forward_ = 0;
        T pdf_reversed_ = 0;

public:
        Surface(const SurfaceIntersection<N, T, Color>& surface,
                const Normals<N, T>& normals,
                const Color& beta,
                const Vector<N, T>& dir_to_prev)
                : surface_(surface),
                  normals_(normals),
                  beta_(beta),
                  dir_to_prev_(dir_to_prev)
        {
                ASSERT(dir_to_prev_.is_unit());
        }

        [[nodiscard]] const Vector<N, T>& dir_to_prev() const
        {
                return dir_to_prev_;
        }

        [[nodiscard]] const Vector<N, T>& pos() const
        {
                return surface_.point();
        }

        [[nodiscard]] const Vector<N, T>& normal() const
        {
                return normals_.shading;
        }

        [[nodiscard]] const Normals<N, T>& normals() const
        {
                return normals_;
        }

        [[nodiscard]] const Color& beta() const
        {
                return beta_;
        }

        template <typename Normal>
        [[nodiscard]] T area_pdf(const T angle_pdf, const Vector<N, T>& next_pos, const Normal& next_normal) const
        {
                return solid_angle_pdf_to_area_pdf(surface_.point(), angle_pdf, next_pos, next_normal);
        }

        [[nodiscard]] bool is_light() const
        {
                return surface_.light_source() != nullptr;
        }

        [[nodiscard]] decltype(auto) light_radiance() const
        {
                ASSERT(surface_.light_source() != nullptr);
                return surface_.light_source()->leave_radiance(dir_to_prev_);
        }

        [[nodiscard]] T light_area_pdf(const Vector<N, T>& next_pos, const Vector<N, T>& next_normal) const
        {
                ASSERT(surface_.light_source() != nullptr);
                const Vector<N, T> l_dir = (next_pos - surface_.point());
                const T l_distance = l_dir.norm();
                const Vector<N, T> l = l_dir / l_distance;
                const T pdf = surface_.light_source()->leave_pdf_dir(l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, next_normal);
        }

        [[nodiscard]] T light_area_origin_pdf() const
        {
                ASSERT(surface_.light_source() != nullptr);
                return surface_.light_source()->leave_pdf_pos(dir_to_prev_);
        }

        template <typename Prev>
        void set_forward_pdf(const Prev& prev, const T angle_pdf)
        {
                pdf_forward_ = prev.area_pdf(angle_pdf, surface_.point(), normals_.shading);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                pdf_reversed_ = next.area_pdf(angle_pdf, surface_.point(), normals_.shading);
        }

        [[nodiscard]] T reversed_pdf(const Vector<N, T>& v, const Surface<N, T, Color>& next) const
        {
                ASSERT(v.is_unit());
                const Vector<N, T> l_dir = (surface_.point() - next.pos());
                const T l_distance = l_dir.norm();
                const Vector<N, T> l = l_dir / l_distance;
                const T pdf = next.angle_pdf(v, l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, normals_.shading);
        }

        void set_reversed_area_pdf(const T pdf)
        {
                pdf_reversed_ = pdf;
        }

        [[nodiscard]] T angle_pdf(const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                ASSERT(v.is_unit());
                ASSERT(l.is_unit());
                return surface_.pdf(normals_.shading, v, l);
        }

        [[nodiscard]] Color brdf(const Vector<N, T>& v, const Vector<N, T>& l) const
        {
                ASSERT(v.is_unit());
                ASSERT(l.is_unit());
                return surface_.brdf(normals_.shading, v, l);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !surface_.is_specular();
        }

        [[nodiscard]] T pdf_reversed() const
        {
                return pdf_reversed_;
        }

        [[nodiscard]] T pdf_forward() const
        {
                return pdf_forward_;
        }
};
}
