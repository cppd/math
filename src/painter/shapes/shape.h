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

#pragma once

#include "../objects.h"
#include "../space/bounding_box.h"

#include <src/numerical/ray.h>

namespace painter
{
// Один объект или структура из объектов, элементами которой
// могут быть объекты или структуры из объектов и т.д.
template <size_t N, typename T>
struct Shape
{
        virtual ~Shape() = default;

        // Для случая структуры из объектов это пересечение луча с границей структуры.
        // Для случая одного объекта это пересечение луча с самим объектом.
        virtual bool intersect_approximate(const Ray<N, T>& r, T* t) const = 0;

        // Для случая структуры из объектов это пересечение луча с объектом.
        // Для случая одного объекта это пересечение луча с самим объектом,
        // уже полученное функцией intersect_approximate.
        virtual bool intersect_precise(
                const Ray<N, T>&,
                T approximate_t,
                T* t,
                const Surface<N, T>** surface,
                const void** intersection_data) const = 0;

        virtual BoundingBox<N, T> bounding_box() const = 0;
};
}
