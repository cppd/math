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

#include <gmpxx.h>

#include <type_traits>

namespace ns
{
template <typename T>
concept Integral =
        (std::is_integral_v<T>) || (std::is_same_v<std::remove_cv_t<T>, unsigned __int128>)
        || (std::is_same_v<std::remove_cv_t<T>, signed __int128>) || (std::is_same_v<std::remove_cv_t<T>, mpz_class>);

template <typename T>
concept FloatingPoint = (std::is_floating_point_v<T>) || (std::is_same_v<std::remove_cv_t<T>, __float128>);

template <typename T>
concept Signed =
        (std::is_signed_v<T>) || (std::is_same_v<std::remove_cv_t<T>, __int128>)
        || (std::is_same_v<std::remove_cv_t<T>, __float128>) || (std::is_same_v<std::remove_cv_t<T>, mpz_class>);

template <typename T>
concept Unsigned = (std::is_unsigned_v<T>) || (std::is_same_v<std::remove_cv_t<T>, unsigned __int128>);
}
