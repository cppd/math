/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <array>
#include <memory>
#include <vector>

namespace ns::geometry
{
template <std::size_t N>
struct ManifoldConstructorCocone
{
        virtual ~ManifoldConstructorCocone() = default;

        virtual const std::vector<Vector<N, float>>& points() const = 0;

        virtual std::vector<std::array<int, N + 1>> delaunay_objects() const = 0;

        virtual void cocone(
                std::vector<vec<N>>* vertex_normals,
                std::vector<std::array<int, N>>* cocone_triangles,
                ProgressRatio* progress) const = 0;
};

template <std::size_t N>
struct ManifoldConstructor
{
        virtual ~ManifoldConstructor() = default;

        virtual const std::vector<Vector<N, float>>& points() const = 0;

        virtual std::vector<std::array<int, N + 1>> delaunay_objects() const = 0;

        virtual void cocone(
                std::vector<vec<N>>* normals,
                std::vector<std::array<int, N>>* facets,
                ProgressRatio* progress) const = 0;
        virtual void bound_cocone(
                double RHO,
                double ALPHA,
                std::vector<vec<N>>* normals,
                std::vector<std::array<int, N>>* facets,
                ProgressRatio* progress) const = 0;
};

template <std::size_t N>
std::unique_ptr<ManifoldConstructor<N>> create_manifold_constructor(
        const std::vector<Vector<N, float>>& source_points,
        ProgressRatio* progress);

template <std::size_t N>
std::unique_ptr<ManifoldConstructorCocone<N>> create_manifold_constructor_cocone(
        const std::vector<Vector<N, float>>& source_points,
        ProgressRatio* progress);
}
