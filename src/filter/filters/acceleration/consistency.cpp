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

#include "consistency.h"

#include <src/filter/settings/instantiation.h>

#include <string>
#include <string_view>

namespace ns::filter::filters::acceleration
{
template <typename T>
std::string make_consistency_string(const Nees<T>& nees, const Nis<T>& nis)
{
        std::string s;

        const auto add = [&](const std::string_view s1, const auto& s2)
        {
                if (s2.empty())
                {
                        return;
                }
                if (!s.empty())
                {
                        s += '\n';
                }
                s += s1;
                s += "; ";
                s += s2.check_string();
        };

        add("NEES position", nees.position);
        add("NEES speed", nees.speed);
        add("NEES angle", nees.angle);
        add("NEES angle r", nees.angle_r);
        add("NIS position", nis.position);
        add("NIS position SDA", nis.position_speed_direction_acceleration);
        add("NIS", nis.nis);

        return s;
}

#define TEMPLATE(T) template std::string make_consistency_string<T>(const Nees<T>&, const Nis<T>&);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
