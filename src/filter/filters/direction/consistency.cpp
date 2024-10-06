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

#include "consistency.h"

#include <src/filter/settings/instantiation.h>

#include <string>

namespace ns::filter::filters::direction
{
template <typename T>
[[nodiscard]] std::string make_consistency_string(const Nees<T>& nees, const Nis<T>& nis)
{
        std::string s;

        const auto add = [&](const auto& text)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += text;
        };

        if (!nees.position.empty())
        {
                add("NEES position; " + nees.position.check_string());
        }

        if (!nees.speed.empty())
        {
                add("NEES speed; " + nees.speed.check_string());
        }

        if (!nees.angle.empty())
        {
                add("NEES angle; " + nees.angle.check_string());
        }

        if (!nis.position.empty())
        {
                add("NIS position; " + nis.position.check_string());
        }

        if (!nis.position_speed_direction.empty())
        {
                add("NIS position SD; " + nis.position_speed_direction.check_string());
        }

        if (!nis.nis.empty())
        {
                add("NIS; " + nis.nis.check_string());
        }

        return s;
}

#define TEMPLATE(T) template std::string make_consistency_string<T>(const Nees<T>&, const Nis<T>&);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
