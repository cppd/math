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

#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::shading
{
template <typename Color>
struct Colors final
{
        Color f0;
        Color rho_ss;

        Colors()
        {
        }

        constexpr Colors(const Color& f0, const Color& rho_ss)
                : f0(f0),
                  rho_ss(rho_ss)
        {
        }
};

template <std::size_t N, typename T, typename Color>
struct Sample final
{
        numerical::Vector<N, T> l;
        T pdf;
        Color brdf;

        Sample()
        {
        }

        constexpr Sample(const numerical::Vector<N, T>& l, const T pdf, const Color& brdf)
                : l(l),
                  pdf(pdf),
                  brdf(brdf)
        {
        }
};
}
