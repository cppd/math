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

#include "com/ray.h"
#include "com/vec.h"

class IntersectionTriangle
{
protected:
        virtual ~IntersectionTriangle() = default;

public:
        virtual bool intersect(const ray3& r, double* t) const = 0;
        virtual const vec3& v0() const = 0;
        virtual const vec3& v1() const = 0;
        virtual const vec3& v2() const = 0;

        IntersectionTriangle() = default;
        IntersectionTriangle(const IntersectionTriangle&) = default;
        IntersectionTriangle(IntersectionTriangle&&) = default;
        IntersectionTriangle& operator=(const IntersectionTriangle&) = default;
        IntersectionTriangle& operator=(IntersectionTriangle&&) = default;
};

class IntersectionRectangle
{
protected:
        virtual ~IntersectionRectangle() = default;

public:
        virtual bool intersect(const ray3& r, double* t) const = 0;
        virtual const vec3& org() const = 0;
        virtual const vec3& e0() const = 0;
        virtual const vec3& e1() const = 0;

        IntersectionRectangle() = default;
        IntersectionRectangle(const IntersectionRectangle&) = default;
        IntersectionRectangle(IntersectionRectangle&&) = default;
        IntersectionRectangle& operator=(const IntersectionRectangle&) = default;
        IntersectionRectangle& operator=(IntersectionRectangle&&) = default;
};

template <size_t N, typename T>
class IntersectionParallelotope
{
protected:
        virtual ~IntersectionParallelotope() = default;

public:
        virtual bool intersect(const Ray<N, T>& r, T* t) const = 0;
        virtual const Vector<N, T>& org() const = 0;
        virtual Vector<N, T> e(unsigned n) const = 0;
        virtual bool inside(const Vector<N, T>& p) const = 0;

        IntersectionParallelotope() = default;
        IntersectionParallelotope(const IntersectionParallelotope&) = default;
        IntersectionParallelotope(IntersectionParallelotope&&) = default;
        IntersectionParallelotope& operator=(const IntersectionParallelotope&) = default;
        IntersectionParallelotope& operator=(IntersectionParallelotope&&) = default;
};

bool shape_intersection(const IntersectionParallelotope<3, double>& p, const IntersectionTriangle& t);
bool shape_intersection(const IntersectionParallelotope<3, double>& p1, const IntersectionParallelotope<3, double>& p2);
