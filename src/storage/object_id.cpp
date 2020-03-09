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

#include "object_id.h"

#include <src/com/error.h>
#include <src/com/print.h>

int object_id_to_int(ObjectId id)
{
        return static_cast<int>(id);
}

ObjectId int_to_object_id(int int_id)
{
        ObjectId id = static_cast<ObjectId>(int_id);

        switch (id)
        {
        case ObjectId::Model:
        case ObjectId::ModelMst:
        case ObjectId::ModelConvexHull:
        case ObjectId::Cocone:
        case ObjectId::CoconeConvexHull:
        case ObjectId::BoundCocone:
        case ObjectId::BoundCoconeConvexHull:
                return id;
        }

        error_fatal("Wrong ObjectId value " + to_string(int_id));
}

const char* object_id_to_text(ObjectId id)
{
        switch (id)
        {
        case ObjectId::Model:
                return "Model";
        case ObjectId::ModelMst:
                return "Model MST";
        case ObjectId::ModelConvexHull:
                return "Model Convex Hull";
        case ObjectId::Cocone:
                return "Cocone";
        case ObjectId::CoconeConvexHull:
                return "Cocone Convex Hull";
        case ObjectId::BoundCocone:
                return "Bound Cocone";
        case ObjectId::BoundCoconeConvexHull:
                return "Bound Cocone Convex Hull";
        }

        error_fatal("Wrong ObjectId value " + to_string(object_id_to_int(id)));
}
