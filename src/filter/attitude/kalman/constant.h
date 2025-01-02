/*
Copyright (C) 2017-2025 Topological Manifold

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

namespace ns::filter::attitude::kalman
{
template <typename T>
inline constexpr T W_THRESHOLD{1e-5}; // rad/s

template <typename T>
inline constexpr T MIN_SIN_Z_MAG{0.1};

inline constexpr unsigned INIT_COUNT{10};
}
