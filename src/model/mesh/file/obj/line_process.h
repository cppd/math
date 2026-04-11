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

#include "data_read.h"
#include "facet.h"
#include "line.h"

#include <src/model/mesh.h>

#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ns::model::mesh::file::obj
{
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
