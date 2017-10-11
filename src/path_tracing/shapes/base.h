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

#include "com/vec.h"
#include "path_tracing/objects.h"

class GeometricTriangle : public GeometricObject
{
protected:
        virtual ~GeometricTriangle() = default;

public:
        virtual const vec3& v0() const = 0;
        virtual const vec3& v1() const = 0;
        virtual const vec3& v2() const = 0;

        GeometricTriangle() = default;
        GeometricTriangle(const GeometricTriangle&) = default;
        GeometricTriangle(GeometricTriangle&&) = default;
        GeometricTriangle& operator=(const GeometricTriangle&) = default;
        GeometricTriangle& operator=(GeometricTriangle&&) = default;
};

class GeometricRectangle : public GeometricObject
{
protected:
        virtual ~GeometricRectangle() = default;

public:
        virtual const vec3& org() const = 0;
        virtual const vec3& e0() const = 0;
        virtual const vec3& e1() const = 0;

        GeometricRectangle() = default;
        GeometricRectangle(const GeometricRectangle&) = default;
        GeometricRectangle(GeometricRectangle&&) = default;
        GeometricRectangle& operator=(const GeometricRectangle&) = default;
        GeometricRectangle& operator=(GeometricRectangle&&) = default;
};

class GeometricParallelepiped : public GeometricObject
{
protected:
        virtual ~GeometricParallelepiped() = default;

public:
        virtual const vec3& org() const = 0;
        virtual vec3 e0() const = 0;
        virtual vec3 e1() const = 0;
        virtual vec3 e2() const = 0;
        virtual bool inside(const vec3& p) const = 0;

        GeometricParallelepiped() = default;
        GeometricParallelepiped(const GeometricParallelepiped&) = default;
        GeometricParallelepiped(GeometricParallelepiped&&) = default;
        GeometricParallelepiped& operator=(const GeometricParallelepiped&) = default;
        GeometricParallelepiped& operator=(GeometricParallelepiped&&) = default;
};
