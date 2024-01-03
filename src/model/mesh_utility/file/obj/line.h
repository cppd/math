/*
Copyright (C) 2017-2024 Topological Manifold

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
#include "data_read.h"
#include "facet.h"

#include <src/com/error.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ns::model::mesh::file::obj
{
template <std::size_t N>
inline constexpr std::size_t MAX_FACETS_PER_LINE = (N == 3 ? 5 : 1);

template <std::size_t N, typename T>
struct Vertex final
{
        Vector<N, T> v;
};

template <std::size_t N, typename T>
struct TextureVertex final
{
        Vector<N - 1, T> v;
};

template <std::size_t N, typename T>
struct Normal final
{
        Vector<N, T> v;
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
                        data.v = Vector<N, T>(0);
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

template <std::size_t N>
class LineProcess final
{
        std::map<std::string, int>* const material_index_;
        std::vector<std::filesystem::path>* const library_names_;
        Mesh<N>* const mesh_;

        int mtl_index_ = -1;
        std::set<std::filesystem::path> unique_library_names_;

public:
        LineProcess(
                std::map<std::string, int>* const material_index,
                std::vector<std::filesystem::path>* const library_names,
                Mesh<N>* const mesh)
                : material_index_(material_index),
                  library_names_(library_names),
                  mesh_(mesh)
        {
        }

        void operator()(const Vertex<N, float>& data)
        {
                mesh_->vertices.push_back(data.v);
        }

        void operator()(const TextureVertex<N, float>& data)
        {
                mesh_->texcoords.push_back(data.v);
        }

        void operator()(const Normal<N, float>& data)
        {
                mesh_->normals.push_back(data.v);
        }

        void operator()(Face<N> data)
        {
                for (int i = 0; i < data.count; ++i)
                {
                        correct_facet_indices<N>(
                                &data.facets[i], mesh_->vertices.size(), mesh_->texcoords.size(),
                                mesh_->normals.size());

                        data.facets[i].material = mtl_index_;

                        mesh_->facets.push_back(std::move(data.facets[i]));
                }
        }

        void operator()(const UseMaterial& data)
        {
                const std::string name{read_name("material", data.second_b, data.second_e)};

                if (const auto iter = material_index_->find(name); iter != material_index_->end())
                {
                        mtl_index_ = iter->second;
                        return;
                }

                mesh_->materials.push_back({.name = name});
                mtl_index_ = mesh_->materials.size() - 1;
                material_index_->emplace(name, mtl_index_);
        }

        void operator()(const MaterialLibrary& data)
        {
                read_library_names(data.second_b, data.second_e, library_names_, &unique_library_names_);
        }
};
}
