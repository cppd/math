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
#include "versions.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
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
                std::optional<ObjectId> parent_object_id;
                Insert(std::shared_ptr<MeshObject<N>>&& object, const std::optional<ObjectId>& parent_object_id)
                        : object(std::move(object)), parent_object_id(parent_object_id)
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

        struct Visibility final
        {
                ObjectId id;
                bool visible;
                Visibility(ObjectId id, bool visible) : id(id), visible(visible)
                {
                }
        };

        using T = std::variant<Insert, Update, Delete, Visibility>;

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

enum class Update
{
        All,
        Alpha,
        Matrix,
        Parameters
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

        float m_alpha = 1;

        Color m_color = Color(Srgb8(150, 170, 150));

        bool m_visible = false;

        //

        template <size_t>
        friend class Writing;

        template <size_t>
        friend class Reading;

        bool m_inserted = false;

        mutable std::shared_mutex m_mutex;

        Versions<Update, Update::All> m_versions;

        void send_event(MeshEvent<N>&& event) noexcept
        {
                try
                {
                        (*m_events)(std::move(event));
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error sending mesh event: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Error sending mesh event");
                }
        }

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

        void insert(const std::optional<ObjectId>& parent_object_id = std::nullopt)
        {
                std::unique_lock m_lock(m_mutex);
                if (!m_inserted)
                {
                        m_inserted = true;
                        send_event(typename MeshEvent<N>::Insert(this->shared_from_this(), parent_object_id));
                }
        }

        ~MeshObject()
        {
                if (m_inserted)
                {
                        send_event(typename MeshEvent<N>::Delete(m_id));
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

        float alpha() const
        {
                return m_alpha;
        }

        void set_alpha(float alpha)
        {
                m_alpha = alpha;
        }

        const Color& color() const
        {
                return m_color;
        }

        void set_color(const Color& color)
        {
                m_color = color;
        }

        void updates(std::optional<int>* version, std::unordered_set<Update>* updates) const
        {
                m_versions.updates(version, updates);
        }

        bool visible() const
        {
                std::shared_lock lock(m_mutex);
                return m_visible;
        }

        void set_visible(bool visible)
        {
                std::unique_lock lock(m_mutex);
                if (m_visible == visible)
                {
                        return;
                }
                m_visible = visible;
                send_event(typename MeshEvent<N>::Visibility(m_id, visible));
        }
};

//

template <size_t N>
class Writing
{
        MeshObject<N>* m_object;
        std::unique_lock<std::shared_mutex> m_lock;

public:
        Writing(MeshObject<N>* object, std::unordered_set<Update>&& updates)
                : m_object(object), m_lock(m_object->m_mutex)
        {
                m_object->m_versions.add(std::move(updates));
        }

        ~Writing()
        {
                if (m_object->m_inserted)
                {
                        m_object->send_event(typename MeshEvent<N>::Update(m_object->shared_from_this()));
                }
        }
};

template <size_t N>
class Reading
{
        std::shared_lock<std::shared_mutex> m_lock;

public:
        Reading(const MeshObject<N>& object) : m_lock(object.m_mutex)
        {
        }
};
}
