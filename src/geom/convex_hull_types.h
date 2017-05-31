/*
Copyright (C) 2017 Topological Manifold

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
#ifndef CONVEX_HULL_TYPES_H
#define CONVEX_HULL_TYPES_H

#include "com/error.h"

enum class ConvexHullComputationType
{
        INTEGER
};

inline const char* to_string(ConvexHullComputationType ct)
{
        switch (ct)
        {
        case ConvexHullComputationType::INTEGER:
                return "integer";
        }
        error_fatal("Unknown convex hull computation type enum value");
}

#endif
