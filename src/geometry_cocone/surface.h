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

#include "geometry/vec.h"
#include "progress/progress.h"

#include <array>
#include <memory>
#include <vector>

template <size_t N>
struct ISurfaceReconstructorCoconeOnly
{
        virtual ~ISurfaceReconstructorCoconeOnly() = default;

        virtual void cocone(std::vector<vec<N>>* vertex_normals, std::vector<std::array<int, N>>* cocone_triangles,
                            ProgressRatio* progress) const = 0;
};

template <size_t N>
struct ISurfaceReconstructor
{
        virtual ~ISurfaceReconstructor() = default;

        virtual void cocone(std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                            ProgressRatio* progress) const = 0;
        virtual void bound_cocone(double RHO, double ALPHA, std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                                  ProgressRatio* progress) const = 0;
};

template <size_t N>
std::unique_ptr<ISurfaceReconstructor<N>> create_surface_reconstructor(const std::vector<Vector<N, float>>& source_points,
                                                                       ProgressRatio* progress);
template <size_t N>
std::unique_ptr<ISurfaceReconstructorCoconeOnly<N>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<N, float>>& source_points, ProgressRatio* progress);

// clang-format off
extern template
std::unique_ptr<ISurfaceReconstructor<2>> create_surface_reconstructor(const std::vector<Vector<2, float>>& source_points,
                                                                       ProgressRatio* progress);
extern template
std::unique_ptr<ISurfaceReconstructor<3>> create_surface_reconstructor(const std::vector<Vector<3, float>>& source_points,
                                                                       ProgressRatio* progress);
extern template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<2>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<2, float>>& source_points, ProgressRatio* progress);
extern template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<3>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<3, float>>& source_points, ProgressRatio* progress);
// clang-format on
