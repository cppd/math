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

#include "counters.h"

#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>

namespace ns::model::mesh::file::obj
{
template <std::size_t N>
inline constexpr std::size_t MAX_FACETS_PER_LINE = (N == 3 ? 5 : 1);

template <std::size_t N, typename T>
struct Vertex final
{
        numerical::Vector<N, T> v;
};

template <std::size_t N, typename T>
struct TextureVertex final
{
        numerical::Vector<N - 1, T> v;
};

template <std::size_t N, typename T>
struct Normal final
{
        numerical::Vector<N, T> v;
};

template <std::size_t N>
struct Face final
{
        std::array<typename Mesh<N>::Facet, MAX_FACETS_PER_LINE<N>> facets;
        int count;
};

struct UseMaterial final
{
        const char* second_b;
        const char* second_e;
};

struct MaterialLibrary final
{
        const char* second_b;
        const char* second_e;
};

template <std::size_t N, typename T>
using Line = std::variant<Vertex<N, T>, TextureVertex<N, T>, Normal<N, T>, Face<N>, UseMaterial, MaterialLibrary>;

//

template <std::size_t N, typename T>
std::optional<Line<N, T>> read_line(
        std::string_view first,
        const char* second_b,
        const char* second_e,
        Counters* counters);
}
