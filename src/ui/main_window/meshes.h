/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/shapes/mesh.h"

#include <map>

enum class MeshType
{
        Model,
        ModelCH,
        Cocone,
        CoconeCH,
        BoundCocone,
        BoundCoconeCH
};

class Meshes
{
        std::map<MeshType, std::shared_ptr<const Mesh>> m_meshes;

public:
        Meshes()
        {
                m_meshes.try_emplace(MeshType::Model);
                m_meshes.try_emplace(MeshType::ModelCH);
                m_meshes.try_emplace(MeshType::Cocone);
                m_meshes.try_emplace(MeshType::CoconeCH);
                m_meshes.try_emplace(MeshType::BoundCocone);
                m_meshes.try_emplace(MeshType::BoundCoconeCH);
        }

        void set(MeshType mesh_type, std::shared_ptr<Mesh>&& mesh)
        {
                ASSERT(m_meshes.count(mesh_type));

                m_meshes[mesh_type] = std::move(mesh);
        }

        void reset(MeshType mesh_type)
        {
                ASSERT(m_meshes.count(mesh_type));

                m_meshes[mesh_type].reset();
        }

        void reset_all()
        {
                for (auto& mesh : m_meshes)
                {
                        mesh.second.reset();
                }
        }

        std::shared_ptr<const Mesh> get(MeshType mesh_type) const
        {
                ASSERT(m_meshes.count(mesh_type));

                return m_meshes.find(mesh_type)->second;
        }
};
