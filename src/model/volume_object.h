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
#include "versions.h"
#include "volume.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <variant>

namespace ns::volume
{
template <std::size_t N>
class VolumeObject;

template <std::size_t N>
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

        struct Erase final
        {
                ObjectId id;
                explicit Erase(ObjectId id) : id(id)
                {
                }
        };

        struct Update final
        {
                std::weak_ptr<VolumeObject<N>> object;
                explicit Update(std::weak_ptr<VolumeObject<N>>&& object) : object(std::move(object))
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

namespace Update
{
enum Flag
{
        Image,
        Matrices,
        Levels,
        VolumeAlphaCoefficient,
        IsosurfaceAlpha,
        Isosurface,
        Isovalue,
        Color,
        Ambient,
        Diffuse,
        Specular,
        SpecularPower
};
using Flags = std::bitset<Flag::SpecularPower + 1>;
}

template <std::size_t N>
class VolumeObject final : public std::enable_shared_from_this<VolumeObject<N>>
{
        template <std::size_t>
        friend class Writing;

        template <std::size_t>
        friend class Reading;

        //

        inline static const std::function<void(VolumeEvent<N>&&)>* m_events = nullptr;

        //

        std::unique_ptr<const Volume<N>> m_volume;
        Matrix<N + 1, N + 1, double> m_matrix;
        std::string m_name;
        ObjectId m_id;

        float m_level_min = 0;
        float m_level_max = 1;

        float m_volume_alpha_coefficient = 1;
        float m_isosurface_alpha = 1;

        bool m_isosurface = false;
        float m_isovalue = 0.5f;

        Color m_color = Color(Srgb8(150, 170, 150));
        float m_ambient = 1;
        float m_diffuse = 1;
        float m_specular = 1;
        float m_specular_power = 50;

        bool m_visible = false;
        bool m_inserted = false;

        mutable std::shared_mutex m_mutex;

        Versions<Update::Flags().size()> m_versions;

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

        //

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

        float volume_alpha_coefficient() const
        {
                return m_volume_alpha_coefficient;
        }

        void set_volume_alpha_coefficient(float coefficient)
        {
                m_volume_alpha_coefficient = coefficient;
        }

        float isosurface_alpha() const
        {
                return m_isosurface_alpha;
        }

        void set_isosurface_alpha(float alpha)
        {
                m_isosurface_alpha = alpha;
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
        static const std::function<void(VolumeEvent<N>&&)>* set_events(
                const std::function<void(VolumeEvent<N>&&)>* events)
        {
                std::swap(m_events, events);
                return events;
        }

        //

        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const Matrix<N + 1, N + 1, double>& matrix,
                std::string name)
                : m_volume(std::move(volume)), m_matrix(matrix), m_name(std::move(name))
        {
                ASSERT(m_volume);
        }

        VolumeObject(const VolumeObject&) = delete;
        VolumeObject& operator=(const VolumeObject&) = delete;
        VolumeObject(VolumeObject&&) = delete;
        VolumeObject& operator=(VolumeObject&&) = delete;

        ~VolumeObject()
        {
                if (m_inserted)
                {
                        send_event(typename VolumeEvent<N>::Erase(m_id));
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
                        send_event(typename VolumeEvent<N>::Insert(this->shared_from_this(), parent_object_id));
                }
        }

        void erase()
        {
                std::unique_lock m_lock(m_mutex);
                if (m_inserted)
                {
                        m_inserted = false;
                        send_event(typename VolumeEvent<N>::Erase(m_id));
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
                send_event(typename VolumeEvent<N>::Visibility(m_id, visible));
        }
};

//

template <std::size_t N>
class Writing final
{
        VolumeObject<N>* m_object;
        std::unique_lock<std::shared_mutex> m_lock;

        Update::Flags m_updates;

public:
        explicit Writing(VolumeObject<N>* object) : m_object(object), m_lock(m_object->m_mutex)
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
                        m_object->send_event(typename VolumeEvent<N>::Update(m_object->weak_from_this()));
                }
        }

        const Volume<N>& volume() const
        {
                return m_object->volume();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_object->matrix();
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                m_updates.set(Update::Matrices);
                m_object->set_matrix(matrix);
        }

        float level_min() const
        {
                return m_object->level_min();
        }

        float level_max() const
        {
                return m_object->level_max();
        }

        void set_levels(float min, float max)
        {
                m_updates.set(Update::Levels);
                m_object->set_levels(min, max);
        }

        float volume_alpha_coefficient() const
        {
                return m_object->volume_alpha_coefficient();
        }

        void set_volume_alpha_coefficient(float coefficient)
        {
                m_updates.set(Update::VolumeAlphaCoefficient);
                m_object->set_volume_alpha_coefficient(coefficient);
        }

        float isosurface_alpha() const
        {
                return m_object->isosurface_alpha();
        }

        void set_isosurface_alpha(float alpha)
        {
                m_updates.set(Update::IsosurfaceAlpha);
                m_object->set_isosurface_alpha(alpha);
        }

        bool isosurface() const
        {
                return m_object->isosurface();
        }

        void set_isosurface(bool enabled)
        {
                m_updates.set(Update::Isovalue);
                m_object->set_isosurface(enabled);
        }

        float isovalue() const
        {
                return m_object->isovalue();
        }

        void set_isovalue(float value)
        {
                m_updates.set(Update::Isovalue);
                m_object->set_isovalue(value);
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

template <std::size_t N>
class Reading final
{
        const VolumeObject<N>* m_object;
        std::shared_lock<std::shared_mutex> m_lock;

public:
        explicit Reading(const VolumeObject<N>& object) : m_object(&object), m_lock(object.m_mutex)
        {
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                m_object->updates(version, updates);
        }

        const Volume<N>& volume() const
        {
                return m_object->volume();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return m_object->matrix();
        }

        float level_min() const
        {
                return m_object->level_min();
        }

        float level_max() const
        {
                return m_object->level_max();
        }

        float volume_alpha_coefficient() const
        {
                return m_object->volume_alpha_coefficient();
        }

        float isosurface_alpha() const
        {
                return m_object->isosurface_alpha();
        }

        bool isosurface() const
        {
                return m_object->isosurface();
        }

        float isovalue() const
        {
                return m_object->isovalue();
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
