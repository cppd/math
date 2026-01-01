/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/color/rgb8.h>
#include <src/com/error.h>
#include <src/numerical/matrix.h>

#include <bitset>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <utility>
#include <variant>

namespace ns::model::volume
{
template <std::size_t N>
class VolumeObject;

namespace event
{
template <std::size_t N>
struct Insert final
{
        std::shared_ptr<VolumeObject<N>> object;
        std::optional<ObjectId> parent_object_id;

        Insert(std::shared_ptr<VolumeObject<N>>&& object, const std::optional<ObjectId>& parent_object_id)
                : object(std::move(object)),
                  parent_object_id(parent_object_id)
        {
        }
};

template <std::size_t N>
struct Erase final
{
        ObjectId id;

        explicit Erase(const ObjectId id)
                : id(id)
        {
        }
};

template <std::size_t N>
struct Update final
{
        std::weak_ptr<VolumeObject<N>> object;

        explicit Update(std::weak_ptr<VolumeObject<N>>&& object)
                : object(std::move(object))
        {
        }
};
}

template <std::size_t N>
using VolumeEvent = std::variant<event::Erase<N>, event::Insert<N>, event::Update<N>>;

template <std::size_t N>
class VolumeEvents
{
protected:
        ~VolumeEvents() = default;

public:
        virtual void send(VolumeEvent<N>&&) const = 0;
};

enum Update
{
        UPDATE_AMBIENT,
        UPDATE_COLOR,
        UPDATE_IMAGE,
        UPDATE_ISOSURFACE,
        UPDATE_ISOSURFACE_ALPHA,
        UPDATE_ISOVALUE,
        UPDATE_LEVELS,
        UPDATE_MATRICES,
        UPDATE_METALNESS,
        UPDATE_ROUGHNESS,
        UPDATE_VISIBILITY,
        UPDATE_VOLUME_ALPHA_COEFFICIENT
};

using Updates = std::bitset<UPDATE_VOLUME_ALPHA_COEFFICIENT + 1>;

template <std::size_t N>
class VolumeObject final : public std::enable_shared_from_this<VolumeObject<N>>
{
        template <std::size_t>
        friend class Writing;

        template <std::size_t>
        friend class Reading;

        //

        class DefaultEvents final : public VolumeEvents<N>
        {
                void send(VolumeEvent<N>&&) const override
                {
                }
        };

        static constexpr DefaultEvents DEFAULT_EVENTS{};

        inline static const VolumeEvents<N>* events_ = &DEFAULT_EVENTS;

        //

        std::unique_ptr<const Volume<N>> volume_;
        std::string name_;
        ObjectId id_;

        bool inserted_ = false;

        numerical::Matrix<N + 1, N + 1, double> matrix_;
        float level_min_ = 0;
        float level_max_ = 1;
        float volume_alpha_coefficient_ = 1;
        float isosurface_alpha_ = 1;
        bool isosurface_ = false;
        float isovalue_ = 0.5f;
        color::Color color_{color::RGB8(220, 255, 220)};
        float ambient_ = 0.1;
        float metalness_ = 0.05;
        float roughness_ = 0.3;
        bool visible_ = false;

        Versions<Updates().size()> versions_;

        mutable std::shared_mutex mutex_;

        void send_event(VolumeEvent<N>&& event) noexcept
        {
                try
                {
                        events_->send(std::move(event));
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
        static void set_events(const VolumeEvents<N>* const events)
        {
                if (events)
                {
                        ASSERT(events_ == &DEFAULT_EVENTS);
                        events_ = events;
                        return;
                }
                events_ = &DEFAULT_EVENTS;
        }

        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const numerical::Matrix<N + 1, N + 1, double>& matrix,
                std::string name)
                : volume_(std::move(volume)),
                  name_(std::move(name)),
                  matrix_(matrix)
        {
                ASSERT(volume_);
        }

        VolumeObject(const VolumeObject&) = delete;
        VolumeObject& operator=(const VolumeObject&) = delete;
        VolumeObject(VolumeObject&&) = delete;
        VolumeObject& operator=(VolumeObject&&) = delete;

        ~VolumeObject()
        {
                if (inserted_)
                {
                        send_event(event::Erase<N>(id_));
                }
        }

        [[nodiscard]] const std::string& name() const
        {
                return name_;
        }

        [[nodiscard]] ObjectId id() const
        {
                return id_;
        }

        void insert(const std::optional<ObjectId>& parent_object_id = std::nullopt)
        {
                const std::unique_lock lock(mutex_);
                if (!inserted_)
                {
                        inserted_ = true;
                        send_event(event::Insert<N>(this->shared_from_this(), parent_object_id));
                }
        }

        void erase()
        {
                std::unique_lock lock(mutex_);
                if (inserted_)
                {
                        inserted_ = false;
                        send_event(event::Erase<N>(id_));
                }
        }
};

template <std::size_t N>
class Writing final
{
        VolumeObject<N>* object_;
        std::unique_lock<std::shared_mutex> lock_;

        Updates updates_;

public:
        explicit Writing(VolumeObject<N>* const object)
                : object_(object),
                  lock_(object_->mutex_)
        {
        }

        Writing(const Writing&) = delete;
        Writing& operator=(const Writing&) = delete;
        Writing(Writing&&) = delete;
        Writing& operator=(Writing&&) = delete;

        ~Writing()
        {
                try
                {
                        if (updates_.none())
                        {
                                return;
                        }
                        object_->versions_.add(updates_);
                        if (object_->inserted_)
                        {
                                object_->send_event(event::Update<N>(object_->weak_from_this()));
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error in volume writing destructor: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Error in volume writing destructor");
                }
        }

        [[nodiscard]] const std::string& name() const
        {
                return object_->name_;
        }

        [[nodiscard]] ObjectId id() const
        {
                return object_->id_;
        }

        [[nodiscard]] const Volume<N>& volume() const
        {
                return *object_->volume_;
        }

        [[nodiscard]] const numerical::Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix_;
        }

        void set_matrix(const numerical::Matrix<N + 1, N + 1, double>& matrix)
        {
                updates_.set(UPDATE_MATRICES);
                object_->matrix_ = matrix;
        }

        [[nodiscard]] float level_min() const
        {
                return object_->level_min_;
        }

        [[nodiscard]] float level_max() const
        {
                return object_->level_max_;
        }

        void set_levels(const float min, const float max)
        {
                updates_.set(UPDATE_LEVELS);
                object_->level_min_ = min;
                object_->level_max_ = max;
        }

        [[nodiscard]] float volume_alpha_coefficient() const
        {
                return object_->volume_alpha_coefficient_;
        }

        void set_volume_alpha_coefficient(const float coefficient)
        {
                updates_.set(UPDATE_VOLUME_ALPHA_COEFFICIENT);
                object_->volume_alpha_coefficient_ = coefficient;
        }

        [[nodiscard]] float isosurface_alpha() const
        {
                return object_->isosurface_alpha_;
        }

        void set_isosurface_alpha(const float alpha)
        {
                updates_.set(UPDATE_ISOSURFACE_ALPHA);
                object_->isosurface_alpha_ = alpha;
        }

        [[nodiscard]] bool isosurface() const
        {
                return object_->isosurface_;
        }

        void set_isosurface(const bool enabled)
        {
                updates_.set(UPDATE_ISOVALUE);
                object_->isosurface_ = enabled;
        }

        [[nodiscard]] float isovalue() const
        {
                return object_->isovalue_;
        }

        void set_isovalue(const float value)
        {
                updates_.set(UPDATE_ISOVALUE);
                object_->isovalue_ = value;
        }

        [[nodiscard]] const color::Color& color() const
        {
                return object_->color_;
        }

        void set_color(const color::Color& color)
        {
                updates_.set(UPDATE_COLOR);
                object_->color_ = color;
        }

        [[nodiscard]] float ambient() const
        {
                return object_->ambient_;
        }

        void set_ambient(const float ambient)
        {
                updates_.set(UPDATE_AMBIENT);
                object_->ambient_ = ambient;
        }

        [[nodiscard]] float metalness() const
        {
                return object_->metalness_;
        }

        void set_metalness(const float metalness)
        {
                updates_.set(UPDATE_METALNESS);
                object_->metalness_ = metalness;
        }

        [[nodiscard]] float roughness() const
        {
                return object_->roughness_;
        }

        void set_roughness(const float roughness)
        {
                updates_.set(UPDATE_ROUGHNESS);
                object_->roughness_ = roughness;
        }

        [[nodiscard]] bool visible() const
        {
                return object_->visible_;
        }

        void set_visible(const bool visible)
        {
                updates_.set(UPDATE_VISIBILITY);
                object_->visible_ = visible;
        }
};

template <std::size_t N>
class Reading final
{
        const VolumeObject<N>* object_;
        std::shared_lock<std::shared_mutex> lock_;

public:
        explicit Reading(const VolumeObject<N>& object)
                : object_(&object),
                  lock_(object.mutex_)
        {
        }

        [[nodiscard]] Updates updates(std::optional<int>* const version) const
        {
                return object_->versions_.updates(version);
        }

        [[nodiscard]] const std::string& name() const
        {
                return object_->name_;
        }

        [[nodiscard]] ObjectId id() const
        {
                return object_->id_;
        }

        [[nodiscard]] const Volume<N>& volume() const
        {
                return *object_->volume_;
        }

        [[nodiscard]] const numerical::Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix_;
        }

        [[nodiscard]] float level_min() const
        {
                return object_->level_min_;
        }

        [[nodiscard]] float level_max() const
        {
                return object_->level_max_;
        }

        [[nodiscard]] float volume_alpha_coefficient() const
        {
                return object_->volume_alpha_coefficient_;
        }

        [[nodiscard]] float isosurface_alpha() const
        {
                return object_->isosurface_alpha_;
        }

        [[nodiscard]] bool isosurface() const
        {
                return object_->isosurface_;
        }

        [[nodiscard]] float isovalue() const
        {
                return object_->isovalue_;
        }

        [[nodiscard]] const color::Color& color() const
        {
                return object_->color_;
        }

        [[nodiscard]] float ambient() const
        {
                return object_->ambient_;
        }

        [[nodiscard]] float metalness() const
        {
                return object_->metalness_;
        }

        [[nodiscard]] float roughness() const
        {
                return object_->roughness_;
        }

        [[nodiscard]] bool visible() const
        {
                return object_->visible_;
        }
};
}
