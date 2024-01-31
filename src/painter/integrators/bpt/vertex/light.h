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
#include "surface.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <cstddef>
#include <optional>

namespace ns::painter::integrators::bpt::vertex
{
template <std::size_t N, typename T, typename Color>
class Light final
{
        const LightSource<N, T, Color>* light_;
        std::optional<numerical::Vector<N, T>> pos_;
        numerical::Vector<N, T> dir_;
        std::optional<numerical::Vector<N, T>> normal_;
        T pdf_forward_;
        T pdf_reversed_ = 0;

public:
        Light(const LightSource<N, T, Color>* const light,
              const std::optional<numerical::Vector<N, T>>& pos,
              const numerical::Vector<N, T>& dir,
              const std::optional<numerical::Vector<N, T>>& normal,
              const T pdf_distribution,
              const T pdf_pos,
              const T pdf_dir)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  pdf_forward_(pdf_distribution * (!light->is_infinite_area() ? pdf_pos : pdf_dir))
        {
        }

        Light(const LightSource<N, T, Color>* const light,
              const T pdf_distribution,
              const T pdf_dir,
              const std::optional<numerical::Vector<N, T>>& pos,
              const numerical::Vector<N, T>& dir,
              const std::optional<numerical::Vector<N, T>>& normal,
              const Surface<N, T, Color>& next)
                : light_(light),
                  pos_(pos),
                  dir_(dir.normalized()),
                  normal_(normal),
                  pdf_forward_(
                          pdf_distribution
                          * (!light->is_infinite_area()
                                     ? light_->leave_pdf_pos(!pos_ ? dir_ : (next.pos() - *pos_).normalized())
                                     : pdf_dir))
        {
        }

        [[nodiscard]] const std::optional<numerical::Vector<N, T>>& pos() const
        {
                return pos_;
        }

        [[nodiscard]] numerical::Vector<N, T> dir_to_light(const numerical::Vector<N, T>& point) const
        {
                if (pos_)
                {
                        return (*pos_ - point).normalized();
                }
                return -dir_;
        }

        [[nodiscard]] const std::optional<numerical::Vector<N, T>>& normal() const
        {
                return normal_;
        }

        [[nodiscard]] T area_pdf(
                const T angle_pdf,
                const numerical::Vector<N, T>& next_pos,
                const numerical::Vector<N, T>& next_normal) const
        {
                if (!pos_)
                {
                        return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
                }
                return solid_angle_pdf_to_area_pdf(*pos_, angle_pdf, next_pos, next_normal);
        }

        [[nodiscard]] T area_pdf(const numerical::Vector<N, T>& next_pos, const numerical::Vector<N, T>& next_normal)
                const
        {
                if (!pos_)
                {
                        return pos_pdf_to_area_pdf(light_->leave_pdf_pos(dir_), dir_, next_normal);
                }
                const numerical::Vector<N, T> l_dir = (next_pos - *pos_);
                const T l_distance = l_dir.norm();
                const numerical::Vector<N, T> l = l_dir / l_distance;
                const T pdf = light_->leave_pdf_dir(l);
                return solid_angle_pdf_to_area_pdf(pdf, l, l_distance, next_normal);
        }

        void set_reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf)
        {
                pdf_reversed_ = this->reversed_pdf(next, angle_pdf);
        }

        [[nodiscard]] T reversed_pdf(const Surface<N, T, Color>& next, const T angle_pdf) const
        {
                if (!pos_)
                {
                        if (light_->is_infinite_area())
                        {
                                return angle_pdf;
                        }
                        return 0;
                }
                ASSERT(!light_->is_infinite_area());
                return next.area_pdf(angle_pdf, *pos_, normal_);
        }

        [[nodiscard]] bool is_connectible() const
        {
                return !light_->is_delta();
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
