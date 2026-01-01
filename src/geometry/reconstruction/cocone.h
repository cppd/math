/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

namespace ns::geometry::reconstruction
{
template <std::size_t N>
class ManifoldConstructorCocone
{
public:
        virtual ~ManifoldConstructorCocone() = default;

        [[nodiscard]] virtual const std::vector<numerical::Vector<N, float>>& points() const = 0;

        [[nodiscard]] virtual std::vector<std::array<int, N + 1>> delaunay_objects() const = 0;

        [[nodiscard]] virtual std::vector<numerical::Vector<N, double>> normals() const = 0;

        [[nodiscard]] virtual std::vector<std::array<int, N>> cocone(progress::Ratio* progress) const = 0;
};

template <std::size_t N>
class ManifoldConstructor
{
public:
        virtual ~ManifoldConstructor() = default;

        [[nodiscard]] virtual const std::vector<numerical::Vector<N, float>>& points() const = 0;

        [[nodiscard]] virtual std::vector<std::array<int, N + 1>> delaunay_objects() const = 0;

        [[nodiscard]] virtual std::vector<numerical::Vector<N, double>> normals() const = 0;

        [[nodiscard]] virtual std::vector<std::array<int, N>> cocone(progress::Ratio* progress) const = 0;

        [[nodiscard]] virtual std::vector<std::array<int, N>> bound_cocone(
                double rho,
                double alpha,
                progress::Ratio* progress) const = 0;
};

template <std::size_t N>
std::unique_ptr<ManifoldConstructor<N>> create_manifold_constructor(
        const std::vector<numerical::Vector<N, float>>& source_points,
        progress::Ratio* progress);

template <std::size_t N>
std::unique_ptr<ManifoldConstructorCocone<N>> create_manifold_constructor_cocone(
        const std::vector<numerical::Vector<N, float>>& source_points,
        progress::Ratio* progress);
}
