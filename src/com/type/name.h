/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <type_traits>

template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, float>, const char*> type_name()
{
        return "float";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, double>, const char*> type_name()
{
        return "double";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, long double>, const char*> type_name()
{
        return "long double";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, __float128>, const char*> type_name()
{
        return "__float128";
}

//

template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, float>, const char*> floating_point_suffix()
{
        return "f";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, double>, const char*> floating_point_suffix()
{
        return "";
}
template <typename T>
constexpr std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, long double>, const char*> floating_point_suffix()
{
        return "l";
}
