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

#include <cstddef>

namespace ns::painter::integrators::bpt::vertex
{
template <std::size_t N, typename T, typename Color>
class Camera final
{
        numerical::Vector<N, T> dir_to_camera_;

public:
        explicit Camera(const numerical::Vector<N, T>& dir)
                : dir_to_camera_(-dir)
        {
                ASSERT(dir_to_camera_.is_unit());
        }

        [[nodiscard]] const numerical::Vector<N, T>& dir_to_camera() const
        {
                return dir_to_camera_;
        }

        [[nodiscard]] T area_pdf(
                [[maybe_unused]] const T angle_pdf,
                const numerical::Vector<N, T>& /*next_pos*/,
                const numerical::Vector<N, T>& /*next_normal*/) const
        {
                ASSERT(angle_pdf == 1);
                return 1;
        }

        [[nodiscard]] bool is_connectible() const
        {
                return true;
        }
};
}
