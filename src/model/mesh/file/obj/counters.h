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

#include <vector>

namespace ns::model::mesh::file::obj
{
struct Counters final
{
        int vertex = 0;
        int texcoord = 0;
        int normal = 0;
        int facet = 0;
};

inline Counters sum_counters(const std::vector<Counters>& counters)
{
        Counters sum;
        for (const Counters& c : counters)
        {
                sum.vertex += c.vertex;
                sum.texcoord += c.texcoord;
                sum.normal += c.normal;
                sum.facet += c.facet;
        }
        return sum;
}
}
