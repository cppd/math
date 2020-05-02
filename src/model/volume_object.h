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
#include <string>
#include <variant>

namespace volume
{
template <size_t N>
class VolumeObject;

template <size_t N>
struct VolumeEvent final
{
        struct Create final
        {
                std::shared_ptr<VolumeObject<N>> object;
                Create(std::shared_ptr<VolumeObject<N>>&& object) : object(std::move(object))
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

template <size_t N>
class VolumeObject final : public std::enable_shared_from_this<VolumeObject<N>>
{
        std::unique_ptr<const Volume<N>> m_volume;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;
        float m_level_min = 0;
        float m_level_max = 1;
        std::function<void(VolumeEvent<N>&&)> m_events;

public:
        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const Matrix<N + 1, N + 1, double>& matrix,
                const std::string& name,
                const std::function<void(VolumeEvent<N>&&)>& events)
                : m_volume(std::move(volume)), m_matrix(matrix), m_name(name), m_events(events)
        {
                ASSERT(m_volume);
        }

        void created()
        {
                m_events(typename VolumeEvent<N>::Create(this->shared_from_this()));
        }

        ~VolumeObject()
        {
                m_events(typename VolumeEvent<N>::Delete(m_id));
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
}
