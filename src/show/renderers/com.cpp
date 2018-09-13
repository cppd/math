/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com.h"

#include "com/error.h"

DrawType draw_type_of_obj(const Obj<3>* obj)
{
        int type_count = 0;

        type_count += obj->facets().size() > 0 ? 1 : 0;
        type_count += obj->points().size() > 0 ? 1 : 0;
        type_count += obj->lines().size() > 0 ? 1 : 0;

        if (type_count > 1)
        {
                error("Supported only faces or points or lines");
        }

        if (obj->facets().size() > 0)
        {
                return DrawType::Triangles;
        }
        else if (obj->points().size() > 0)
        {
                return DrawType::Points;
        }
        else if (obj->lines().size() > 0)
        {
                return DrawType::Lines;
        }
        else
        {
                error("Faces or points or lines not found");
        }
}
