/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "name.h"

#include <gmpxx.h>

namespace ns
{
template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, mpz_class>)
{
        return "mpz_class";
}

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, mpq_class>)
{
        return "mpq_class";
}

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, mpf_class>)
{
        return "mpf_class";
}

template <typename T>
constexpr const char* type_bit_name() requires(std::is_same_v<std::remove_cv_t<T>, mpz_class>)
{
        return "mpz_class";
}
}
