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
#include <shared_mutex>
#include <string>
#include <variant>

namespace mesh
{
template <size_t N>
class MeshObject;

template <size_t N>
struct MeshEvent final
{
        struct Insert final
        {
                std::shared_ptr<MeshObject<N>> object;
                Insert(std::shared_ptr<MeshObject<N>>&& object) : object(std::move(object))
                {
                }
        };

        struct Update final
        {
                std::shared_ptr<MeshObject<N>> object;
                Update(std::shared_ptr<MeshObject<N>>&& object) : object(std::move(object))
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

        using T = std::variant<Insert, Update, Delete>;

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
        inline static const std::function<void(MeshEvent<N>&&)>* m_events = nullptr;

        //

        std::unique_ptr<const Mesh<N>> m_mesh;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;

        //

        bool m_inserted = false;

        mutable std::shared_mutex m_mutex;

public:
        static void set_events(const std::function<void(MeshEvent<N>&&)>* events)
        {
                m_events = events;
        }

        MeshObject(
                std::unique_ptr<const Mesh<N>>&& mesh,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name)
                : m_mesh(std::move(mesh)), m_matrix(matrix), m_name(name)
        {
                ASSERT(m_mesh);
        }

        void insert()
        {
                std::unique_lock m_lock(m_mutex);
                if (!m_inserted)
                {
                        m_inserted = true;
                        (*m_events)(typename MeshEvent<N>::Insert(this->shared_from_this()));
                }
        }

        ~MeshObject()
        {
                if (m_inserted)
                {
                        (*m_events)(typename MeshEvent<N>::Delete(m_id));
                }
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
