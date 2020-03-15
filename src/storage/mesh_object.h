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

#include "object_id.h"

#include <src/com/error.h>
#include <src/model/mesh.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <string>

template <size_t N>
class MeshObject
{
        std::unique_ptr<const mesh::Mesh<N>> m_mesh;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;

public:
        MeshObject(
                std::unique_ptr<const mesh::Mesh<N>>&& mesh,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name,
                const ObjectId& id)
                : m_mesh(std::move(mesh)), m_matrix(matrix), m_name(name), m_id(id)
        {
                ASSERT(m_mesh);
        }

        const mesh::Mesh<N>& mesh() const
        {
                return *m_mesh;
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_matrix;
        }

        const std::string& name() const
        {
                return m_name;
        }

        const ObjectId& id() const
        {
                return m_id;
        }
};
