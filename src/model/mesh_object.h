/*
Copyright (C) 2017-2021 Topological Manifold

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

namespace ns::mesh
{
template <std::size_t N>
class MeshObject;

template <std::size_t N>
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

        struct Erase final
        {
                ObjectId id;
                explicit Erase(ObjectId id) : id(id)
                {
                }
        };

        struct Update final
        {
                std::weak_ptr<MeshObject<N>> object;
                explicit Update(std::weak_ptr<MeshObject<N>>&& object) : object(std::move(object))
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

        using T = std::variant<Insert, Erase, Update, Visibility>;

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

namespace Update
{
enum Flag
{
        Mesh,
        Alpha,
        Matrix,
        Color,
        Ambient,
        Metalness,
        Roughness
};
using Flags = std::bitset<Flag::Roughness + 1>;
}

template <std::size_t N>
class MeshObject final : public std::enable_shared_from_this<MeshObject<N>>
{
        template <std::size_t>
        friend class Writing;

        template <std::size_t>
        friend class Reading;

        //

        inline static const std::function<void(MeshEvent<N>&&)>* m_events = nullptr;

        //

        std::unique_ptr<const Mesh<N>> m_mesh;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;

        float m_alpha = 1;

        Color m_color = Color(Srgb8(235, 255, 235));
        float m_ambient = 0.3;
        float m_metalness = 0.1;
        float m_roughness = 0.2;

        bool m_visible = false;
        bool m_inserted = false;

        mutable std::shared_mutex m_mutex;

        Versions<Update::Flags().size()> m_versions;

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

        //

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

        float ambient() const
        {
                return m_ambient;
        }

        void set_ambient(float ambient)
        {
                m_ambient = ambient;
        }

        float metalness() const
        {
                return m_metalness;
        }

        void set_metalness(float metalness)
        {
                m_metalness = metalness;
        }

        float roughness() const
        {
                return m_roughness;
        }

        void set_roughness(float roughness)
        {
                m_roughness = roughness;
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                m_versions.updates(version, updates);
        }

public:
        static const std::function<void(MeshEvent<N>&&)>* set_events(const std::function<void(MeshEvent<N>&&)>* events)
        {
                std::swap(m_events, events);
                return events;
        }

        MeshObject(std::unique_ptr<const Mesh<N>>&& mesh, const Matrix<N + 1, N + 1, double>& matrix, std::string name)
                : m_mesh(std::move(mesh)), m_matrix(matrix), m_name(std::move(name))
        {
                ASSERT(m_mesh);
        }

        MeshObject(const MeshObject&) = delete;
        MeshObject& operator=(const MeshObject&) = delete;
        MeshObject(MeshObject&&) = delete;
        MeshObject& operator=(MeshObject&&) = delete;

        ~MeshObject()
        {
                if (m_inserted)
                {
                        send_event(typename MeshEvent<N>::Erase(m_id));
                }
        }

        const std::string& name() const
        {
                return m_name;
        }

        const ObjectId& id() const
        {
                return m_id;
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

        void erase()
        {
                std::unique_lock m_lock(m_mutex);
                if (m_inserted)
                {
                        m_inserted = false;
                        send_event(typename MeshEvent<N>::Erase(m_id));
                }
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
                if (m_inserted)
                {
                        send_event(typename MeshEvent<N>::Visibility(m_id, visible));
                }
        }
};

//

template <std::size_t N>
class Writing final
{
        MeshObject<N>* m_object;
        std::unique_lock<std::shared_mutex> m_lock;

        Update::Flags m_updates;

public:
        explicit Writing(MeshObject<N>* object) : m_object(object), m_lock(m_object->m_mutex)
        {
        }

        Writing(const Writing&) = delete;
        Writing& operator=(const Writing&) = delete;
        Writing(Writing&&) = delete;
        Writing& operator=(Writing&&) = delete;

        ~Writing()
        {
                if (m_updates.none())
                {
                        return;
                }
                m_object->m_versions.add(m_updates);
                if (m_object->m_inserted)
                {
                        m_object->send_event(typename MeshEvent<N>::Update(m_object->weak_from_this()));
                }
        }

        const std::string& name() const
        {
                return m_object->name();
        }

        const ObjectId& id() const
        {
                return m_object->id();
        }

        const Mesh<N>& mesh() const
        {
                return m_object->mesh();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_object->matrix();
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                m_updates.set(Update::Matrix);
                m_object->set_matrix(matrix);
        }

        float alpha() const
        {
                return m_object->alpha();
        }

        void set_alpha(float alpha)
        {
                m_updates.set(Update::Alpha);
                m_object->set_alpha(alpha);
        }

        const Color& color() const
        {
                return m_object->color();
        }

        void set_color(const Color& color)
        {
                m_updates.set(Update::Color);
                m_object->set_color(color);
        }

        float ambient() const
        {
                return m_object->ambient();
        }

        void set_ambient(float ambient)
        {
                m_updates.set(Update::Ambient);
                m_object->set_ambient(ambient);
        }

        float metalness() const
        {
                return m_object->metalness();
        }

        void set_metalness(float metalness)
        {
                m_updates.set(Update::Metalness);
                m_object->set_metalness(metalness);
        }

        float roughness() const
        {
                return m_object->roughness();
        }

        void set_roughness(float roughness)
        {
                m_updates.set(Update::Roughness);
                m_object->set_roughness(roughness);
        }
};

template <std::size_t N>
class Reading final
{
        const MeshObject<N>* m_object;
        std::shared_lock<std::shared_mutex> m_lock;

public:
        explicit Reading(const MeshObject<N>& object) : m_object(&object), m_lock(object.m_mutex)
        {
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                m_object->updates(version, updates);
        }

        const std::string& name() const
        {
                return m_object->name();
        }

        const ObjectId& id() const
        {
                return m_object->id();
        }

        const Mesh<N>& mesh() const
        {
                return m_object->mesh();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_object->matrix();
        }

        float alpha() const
        {
                return m_object->alpha();
        }

        const Color& color() const
        {
                return m_object->color();
        }

        float ambient() const
        {
                return m_object->ambient();
        }

        float metalness() const
        {
                return m_object->metalness();
        }

        float roughness() const
        {
                return m_object->roughness();
        }
};
}
