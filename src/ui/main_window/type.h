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

#pragma once

// Идентификаторы объектов как числа, а не как enum class
enum ObjectIdentifier
{
        OBJECT_MODEL,
        OBJECT_MODEL_MST,
        OBJECT_MODEL_CONVEX_HULL,
        OBJECT_COCONE,
        OBJECT_COCONE_CONVEX_HULL,
        OBJECT_BOUND_COCONE,
        OBJECT_BOUND_COCONE_CONVEX_HULL
};

enum class SourceType
{
        File,
        Repository
};
