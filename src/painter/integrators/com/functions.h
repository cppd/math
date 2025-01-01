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

#include <optional>

namespace ns::painter::integrators::com
{
template <typename Dst, typename Src>
void add_optional(std::optional<Dst>* const dst, Src&& src)
        requires requires { **dst = src; }
{
        if (*dst)
        {
                **dst += std::forward<Src>(src);
        }
        else
        {
                *dst = std::forward<Src>(src);
        }
}

template <typename Dst, typename Src>
void add_optional(std::optional<Dst>* const dst, Src&& src)
        requires requires { **dst = *src; }
{
        if (src)
        {
                add_optional(dst, *std::forward<Src>(src));
        }
}
}
