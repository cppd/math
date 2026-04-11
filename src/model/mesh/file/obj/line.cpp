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

#include <src/com/error.h>
#include <src/model/mesh/file/data_read.h>
#include <src/model/mesh/file/obj/counters.h>
#include <src/numerical/vector_object.h>
#include <src/settings/instantiation.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace ns::model::mesh::file::obj
{
template <std::size_t N, typename T>
std::optional<Line<N, T>> read_line(
        const std::string_view first,
        const char* const second_b,
        const char* const second_e,
        Counters* const counters)
{
        if (first == "v")
        {
                Vertex<N, T> data;
                read(second_b, &data.v);
                ++counters->vertex;
                return data;
        }

        if (first == "vt")
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

        if (first == "vn")
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

        if (first == "f")
        {
                Face<N> data;
                read_facets<N>(second_b, second_e, &data.facets, &data.count);
                ++counters->facet;
                return data;
        }

        if (first == "usemtl")
        {
                UseMaterial data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }

        if (first == "mtllib")
        {
                MaterialLibrary data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }

        return std::nullopt;
}

#define TEMPLATE(N, T) \
        template std::optional<Line<(N), T>> read_line(const std::string_view, const char*, const char*, Counters*);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
