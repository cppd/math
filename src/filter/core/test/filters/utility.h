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

#include <src/filter/core/test/measurements.h>

#include <optional>

namespace ns::filter::core::test::filters
{
template <typename Filter>
bool filter_update(
        Filter* const filter,
        const Measurements<typename Filter::Type>& m,
        const std::optional<typename Filter::Type> gate)
{
        ASSERT(filter);
        ASSERT(m.x || m.v);

        if (m.x)
        {
                if (m.v)
                {
                        filter->update_position_speed(m.x->value, m.x->variance, m.v->value, m.v->variance, gate);
                        return true;
                }
                filter->update_position(m.x->value, m.x->variance, gate);
                return true;
        }
        if (m.v)
        {
                filter->update_speed(m.v->value, m.v->variance, gate);
                return true;
        }
        return false;
}
}
