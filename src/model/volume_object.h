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
#include "volume.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <variant>

namespace volume
{
template <size_t N>
class VolumeObject;

template <size_t N>
struct VolumeEvent final
{
        struct Insert final
        {
                std::shared_ptr<VolumeObject<N>> object;
                std::optional<ObjectId> parent_object_id;
                Insert(std::shared_ptr<VolumeObject<N>>&& object, const std::optional<ObjectId>& parent_object_id)
                        : object(std::move(object)), parent_object_id(parent_object_id)
                {
                }
        };

        struct Update final
        {
                std::shared_ptr<VolumeObject<N>> object;
                Update(std::shared_ptr<VolumeObject<N>>&& object) : object(std::move(object))
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

        template <typename Type, typename = std::enable_if_t<!std::is_same_v<VolumeEvent, std::remove_cvref_t<Type>>>>
        VolumeEvent(Type&& arg) : m_data(std::forward<Type>(arg))
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
        Image,
        Parameters,
        Matrices
};

template <size_t N>
class VolumeObject final : public std::enable_shared_from_this<VolumeObject<N>>
{
        inline static const std::function<void(VolumeEvent<N>&&)>* m_events = nullptr;

        //

        std::unique_ptr<const Volume<N>> m_volume;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;
        float m_level_min = 0;
        float m_level_max = 1;
        float m_transparency = 1;

        bool m_isosurface = false;
        float m_isovalue = 0.5f;

        //

        template <size_t>
        friend class WritingUpdates;

        template <size_t>
        friend class ReadingUpdates;

        template <size_t>
        friend class Reading;

        bool m_inserted = false;

        mutable std::shared_mutex m_mutex;
        mutable std::unordered_set<Update> m_updates{Update::All};

        void send_event(VolumeEvent<N>&& event) noexcept
        {
                try
                {
                        (*m_events)(std::move(event));
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error sending volume event: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Error sending volume event");
                }
        }

public:
        static void set_events(const std::function<void(VolumeEvent<N>&&)>* events)
        {
                m_events = events;
        }

        //

        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name)
                : m_volume(std::move(volume)), m_matrix(matrix), m_name(name)
        {
                ASSERT(m_volume);
        }

        void insert(const std::optional<ObjectId>& parent_object_id = std::nullopt)
        {
                std::unique_lock m_lock(m_mutex);
                if (!m_inserted)
                {
                        m_updates = {Update::All};
                        m_inserted = true;
                        send_event(typename VolumeEvent<N>::Insert(this->shared_from_this(), parent_object_id));
                }
        }

        ~VolumeObject()
        {
                if (m_inserted)
                {
                        send_event(typename VolumeEvent<N>::Delete(m_id));
                }
        }

        const Volume<N>& volume() const
        {
                return *m_volume;
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

        float level_min() const
        {
                return m_level_min;
        }

        float level_max() const
        {
                return m_level_max;
        }

        void set_levels(float min, float max)
        {
                m_level_min = min;
                m_level_max = max;
        }

        float transparency() const
        {
                return m_transparency;
        }

        void set_transparency(float transparency)
        {
                m_transparency = transparency;
        }

        bool isosurface() const
        {
                return m_isosurface;
        }

        void set_isosurface(bool enabled)
        {
                m_isosurface = enabled;
        }

        float isovalue() const
        {
                return m_isovalue;
        }

        void set_isovalue(float value)
        {
                m_isovalue = value;
        }
};

//

template <size_t N>
class WritingUpdates
{
        VolumeObject<N>* m_object;
        std::unique_lock<std::shared_mutex> m_lock;

public:
        WritingUpdates(VolumeObject<N>* object, std::unordered_set<Update>&& updates)
                : m_object(object), m_lock(m_object->m_mutex)
        {
                m_object->m_updates.merge(std::move(updates));
        }

        ~WritingUpdates()
        {
                if (m_object->m_inserted)
                {
                        m_object->send_event(typename VolumeEvent<N>::Update(m_object->shared_from_this()));
                }
        }
};

template <size_t N>
class ReadingUpdates
{
        const VolumeObject<N>& m_object;
        std::unique_lock<std::shared_mutex> m_lock;

public:
        ReadingUpdates(const VolumeObject<N>& object) : m_object(object), m_lock(m_object.m_mutex)
        {
        }

        ~ReadingUpdates()
        {
                m_object.m_updates.clear();
        }

        const std::unordered_set<Update>& updates() const
        {
                return m_object.m_updates;
        }
};

template <size_t N>
class Reading
{
        std::shared_lock<std::shared_mutex> m_lock;

public:
        Reading(const VolumeObject<N>& object) : m_lock(object.m_mutex)
        {
        }
};
}
