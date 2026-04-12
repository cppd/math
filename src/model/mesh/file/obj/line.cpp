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

#include "line.h"

#include "facet.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/model/mesh/file/data_read.h>
#include <src/model/mesh/file/obj/counters.h>
#include <src/numerical/vector_object.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ns::model::mesh::file::obj
{
namespace
{
enum class LineType
{
        V,
        VT,
        VN,
        F,
        USEMTL,
        MTLLIB
};

constexpr std::array LINE_TYPES = std::to_array<std::pair<std::string_view, LineType>>({
        {     "v",      LineType::V},
        {    "vt",     LineType::VT},
        {    "vn",     LineType::VN},
        {     "f",      LineType::F},
        {"usemtl", LineType::USEMTL},
        {"mtllib", LineType::MTLLIB},
});

std::optional<LineType> find_line_type(const std::string_view& first)
{
        for (const auto& p : LINE_TYPES)
        {
                if (p.first == first)
                {
                        return p.second;
                }
        }
        return std::nullopt;
}
}

template <std::size_t N, typename T>
std::optional<Line<N, T>> read_line(
        const std::string_view first,
        const char* const second_b,
        const char* const second_e,
        Counters* const counters)
{
        const auto line_type = find_line_type(first);
        if (!line_type)
        {
                return std::nullopt;
        }

        switch (*line_type)
        {
        case LineType::V:
        {
                Vertex<N, T> data;
                read(second_b, &data.v);
                ++counters->vertex;
                return data;
        }
        case LineType::VT:
        {
                TextureVertex<N, T> data;
                std::optional<T> t;
                read(second_b, &data.v, &t);
                if (t && !(*t == 0))
                {
                        error(std::to_string(N) + "-dimensional textures are not supported");
                }
                ++counters->texcoord;
                return data;
        }
        case LineType::VN:
        {
                Normal<N, T> data;
                read(second_b, &data.v);
                data.v.normalize();
                if (!is_finite(data.v))
                {
                        data.v = numerical::Vector<N, T>(0);
                }
                ++counters->normal;
                return data;
        }
        case LineType::F:
        {
                Face<N> data;
                read_facets<N>(second_b, second_e, &data.facets, &data.count);
                ++counters->facet;
                return data;
        }
        case LineType::USEMTL:
        {
                UseMaterial data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }
        case LineType::MTLLIB:
        {
                MaterialLibrary data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }
        }

        error_fatal("Unknown line type " + to_string(enum_to_int(*line_type)));
}

#define TEMPLATE(N, T) \
        template std::optional<Line<(N), T>> read_line(const std::string_view, const char*, const char*, Counters*);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
