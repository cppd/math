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

#include "com/vec.h"

#include <memory>
#include <string>
#include <vector>

template <size_t N>
struct ObjectRepository
{
        virtual ~ObjectRepository() = default;

        virtual std::vector<Vector<N, float>> ellipsoid(unsigned point_count) const = 0;
        virtual std::vector<Vector<N, float>> ellipsoid_bound(unsigned point_count) const = 0;
        virtual std::vector<Vector<N, float>> sphere_with_notch(unsigned point_count) const = 0;
        virtual std::vector<Vector<N, float>> sphere_with_notch_bound(unsigned point_count) const = 0;

        virtual std::vector<std::string> point_object_names() const = 0;
        virtual std::vector<Vector<N, float>> point_object(const std::string& object_name, unsigned point_count) const = 0;
};

template <size_t N>
std::unique_ptr<ObjectRepository<N>> create_object_repository();
