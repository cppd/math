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

#include "mesh.h"
#include "object_id.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <string>
#include <variant>

namespace mesh
{
template <size_t N>
class MeshObject;

template <size_t N>
struct MeshEvent final
{
        struct Create final
        {
                std::shared_ptr<MeshObject<N>> object;
                Create(std::shared_ptr<MeshObject<N>>&& object) : object(std::move(object))
                {
                }
        };

        struct Delete final
        {
                ObjectId id;
                Delete(ObjectId id) : id(id)
                {
                }
        };

        using T = std::variant<Create, Delete>;

        template <typename Type, typename = std::enable_if_t<!std::is_same_v<MeshEvent, std::remove_cvref_t<Type>>>>
        MeshEvent(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};

template <size_t N>
class MeshObject final : public std::enable_shared_from_this<MeshObject<N>>
{
        std::unique_ptr<const Mesh<N>> m_mesh;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;
        std::function<void(MeshEvent<N>&&)> m_events;

public:
        MeshObject(
                std::unique_ptr<const Mesh<N>>&& mesh,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name,
                const std::function<void(MeshEvent<N>&&)>& events)
                : m_mesh(std::move(mesh)), m_matrix(matrix), m_name(name), m_events(events)
        {
                ASSERT(m_mesh);
        }

        void created()
        {
                m_events(typename MeshEvent<N>::Create(this->shared_from_this()));
        }

        ~MeshObject()
        {
                m_events(typename MeshEvent<N>::Delete(m_id));
        }

        const Mesh<N>& mesh() const
        {
                return *m_mesh;
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_matrix;
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                m_matrix = matrix;
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
}
