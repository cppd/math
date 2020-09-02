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
                explicit Update(std::shared_ptr<MeshObject<N>>&& object) : object(std::move(object))
                {
                }
        };

        struct Delete final
        {
                ObjectId id;
                explicit Delete(ObjectId id) : id(id)
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

namespace Update
{
enum Flag
{
        Mesh,
        Alpha,
        Matrix,
        Color,
        Ambient,
        Diffuse,
        Specular,
        SpecularPower
};
using Flags = std::bitset<Flag::SpecularPower + 1>;
}

template <size_t N>
class MeshObject final : public std::enable_shared_from_this<MeshObject<N>>
{
        template <size_t>
        friend class Writing;

        template <size_t>
        friend class Reading;

        //

        inline static const std::function<void(MeshEvent<N>&&)>* m_events = nullptr;

        //

        std::unique_ptr<const Mesh<N>> m_mesh;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;

        float m_alpha = 1;
        Color m_color = Color(Srgb8(150, 170, 150));
        float m_ambient = 1;
        float m_diffuse = 1;
        float m_specular = 1;
        float m_specular_power = 50;

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

        float diffuse() const
        {
                return m_diffuse;
        }

        void set_diffuse(float diffuse)
        {
                m_diffuse = diffuse;
        }

        float specular() const
        {
                return m_specular;
        }

        void set_specular(float specular)
        {
                m_specular = specular;
        }

        float specular_power() const
        {
                return m_specular_power;
        }

        void set_specular_power(float specular_power)
        {
                m_specular_power = specular_power;
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                m_versions.updates(version, updates);
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

        ~MeshObject()
        {
                if (m_inserted)
                {
                        send_event(typename MeshEvent<N>::Delete(m_id));
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

template <size_t N>
class Writing final
{
        MeshObject<N>* m_object;
        std::unique_lock<std::shared_mutex> m_lock;

        Update::Flags m_updates;

public:
        explicit Writing(MeshObject<N>* object) : m_object(object), m_lock(m_object->m_mutex)
        {
        }

        ~Writing()
        {
                if (m_updates.none())
                {
                        return;
                }
                m_object->m_versions.add(std::move(m_updates));
                if (m_object->m_inserted)
                {
                        m_object->send_event(typename MeshEvent<N>::Update(m_object->shared_from_this()));
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

        float diffuse() const
        {
                return m_object->diffuse();
        }

        void set_diffuse(float diffuse)
        {
                m_updates.set(Update::Diffuse);
                m_object->set_diffuse(diffuse);
        }

        float specular() const
        {
                return m_object->specular();
        }

        void set_specular(float specular)
        {
                m_updates.set(Update::Specular);
                m_object->set_specular(specular);
        }

        float specular_power() const
        {
                return m_object->specular_power();
        }

        void set_specular_power(float specular_power)
        {
                m_updates.set(Update::SpecularPower);
                m_object->set_specular_power(specular_power);
        }
};

template <size_t N>
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

        float diffuse() const
        {
                return m_object->diffuse();
        }

        float specular() const
        {
                return m_object->specular();
        }

        float specular_power() const
        {
                return m_object->specular_power();
        }
};
}
