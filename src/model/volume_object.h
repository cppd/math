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

        using T = std::variant<Update, Delete>;

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
        Levels,
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

        //

        template <size_t>
        friend class WritingUpdates;

        template <size_t>
        friend class ReadingUpdates;

        template <size_t>
        friend class Reading;

        mutable std::shared_mutex m_mutex;
        mutable std::unordered_set<Update> m_updates{Update::All};

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

        void update_all()
        {
                std::unique_lock m_lock(m_mutex);
                m_updates = {Update::All};
                (*m_events)(typename VolumeEvent<N>::Update(this->shared_from_this()));
        }

        ~VolumeObject()
        {
                (*m_events)(typename VolumeEvent<N>::Delete(m_id));
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

        ~WritingUpdates() noexcept(false)
        {
                // Нужно снять блокировку до вызова функции событий, так как
                // обработчики событий могут читать изменения в этом же потоке.
                m_lock.unlock();
                (*m_object->m_events)(typename VolumeEvent<N>::Update(m_object->shared_from_this()));
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
