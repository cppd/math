/*
Copyright (C) 2017-2023 Topological Manifold

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
        requires std::is_same_v<std::remove_cv_t<T>, mpz_class>
[[nodiscard]] constexpr const char* type_name()
{
        return "mpz_class";
}

template <typename T>
        requires std::is_same_v<std::remove_cv_t<T>, mpq_class>
[[nodiscard]] constexpr const char* type_name()
{
        return "mpq_class";
}

template <typename T>
        requires std::is_same_v<std::remove_cv_t<T>, mpf_class>
[[nodiscard]] constexpr const char* type_name()
{
        return "mpf_class";
}

template <typename T>
        requires std::is_same_v<std::remove_cv_t<T>, mpz_class>
[[nodiscard]] constexpr const char* type_bit_name()
{
        return "mpz_class";
}
}
