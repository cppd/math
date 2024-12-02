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

namespace ns::filter::attitude::ekf
{
template <typename T>
inline constexpr T W_THRESHOLD{1e-5}; // rad/s

template <typename T>
inline constexpr T MIN_ACCELERATION = 9.0; // m/s/s

template <typename T>
inline constexpr T MAX_ACCELERATION = 10.6; // m/s/s

template <typename T>
inline constexpr T MIN_MAGNETIC_FIELD = 20; // uT

template <typename T>
inline constexpr T MAX_MAGNETIC_FIELD = 70; // uT

}
