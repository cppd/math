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

        template <typename Type>
        VolumeEvent(Type&& arg) requires(!std::is_same_v<VolumeEvent, std::remove_cvref_t<Type>>)
                : data_(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return data_;
        }

private:
        T data_;
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
        Metalness,
        Roughness
};
using Flags = std::bitset<Flag::Roughness + 1>;
}

template <std::size_t N>
class VolumeObject final : public std::enable_shared_from_this<VolumeObject<N>>
{
        template <std::size_t>
        friend class Writing;

        template <std::size_t>
        friend class Reading;

        //

        inline static const std::function<void(VolumeEvent<N>&&)>* events_ = nullptr;

        //

        std::unique_ptr<const Volume<N>> volume_;
        Matrix<N + 1, N + 1, double> matrix_;
        std::string name_;
        ObjectId id_;

        float level_min_ = 0;
        float level_max_ = 1;

        float volume_alpha_coefficient_ = 1;
        float isosurface_alpha_ = 1;

        bool isosurface_ = false;
        float isovalue_ = 0.5f;

        color::Color color_ = RGB8(220, 255, 220);
        float ambient_ = 0.2;
        float metalness_ = 0.05;
        float roughness_ = 0.3;

        bool visible_ = false;
        bool inserted_ = false;

        mutable std::shared_mutex mutex_;

        Versions<Update::Flags().size()> versions_;

        void send_event(VolumeEvent<N>&& event) noexcept
        {
                try
                {
                        (*events_)(std::move(event));
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
                return *volume_;
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return matrix_;
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                matrix_ = matrix;
        }

        float level_min() const
        {
                return level_min_;
        }

        float level_max() const
        {
                return level_max_;
        }

        void set_levels(float min, float max)
        {
                level_min_ = min;
                level_max_ = max;
        }

        float volume_alpha_coefficient() const
        {
                return volume_alpha_coefficient_;
        }

        void set_volume_alpha_coefficient(float coefficient)
        {
                volume_alpha_coefficient_ = coefficient;
        }

        float isosurface_alpha() const
        {
                return isosurface_alpha_;
        }

        void set_isosurface_alpha(float alpha)
        {
                isosurface_alpha_ = alpha;
        }

        bool isosurface() const
        {
                return isosurface_;
        }

        void set_isosurface(bool enabled)
        {
                isosurface_ = enabled;
        }

        float isovalue() const
        {
                return isovalue_;
        }

        void set_isovalue(float value)
        {
                isovalue_ = value;
        }

        const color::Color& color() const
        {
                return color_;
        }

        void set_color(const color::Color& color)
        {
                color_ = color;
        }

        float ambient() const
        {
                return ambient_;
        }

        void set_ambient(float ambient)
        {
                ambient_ = ambient;
        }

        float metalness() const
        {
                return metalness_;
        }

        void set_metalness(float metalness)
        {
                metalness_ = metalness;
        }

        float roughness() const
        {
                return roughness_;
        }

        void set_roughness(float roughness)
        {
                roughness_ = roughness;
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                versions_.updates(version, updates);
        }

public:
        static const std::function<void(VolumeEvent<N>&&)>* set_events(
                const std::function<void(VolumeEvent<N>&&)>* events)
        {
                std::swap(events_, events);
                return events;
        }

        //

        VolumeObject(
                std::unique_ptr<const Volume<N>>&& volume,
                const Matrix<N + 1, N + 1, double>& matrix,
                std::string name)
                : volume_(std::move(volume)), matrix_(matrix), name_(std::move(name))
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
                        send_event(typename VolumeEvent<N>::Erase(id_));
                }
        }

        const std::string& name() const
        {
                return name_;
        }

        const ObjectId& id() const
        {
                return id_;
        }

        void insert(const std::optional<ObjectId>& parent_object_id = std::nullopt)
        {
                std::unique_lock lock(mutex_);
                if (!inserted_)
                {
                        inserted_ = true;
                        send_event(typename VolumeEvent<N>::Insert(this->shared_from_this(), parent_object_id));
                }
        }

        void erase()
        {
                std::unique_lock lock_(mutex_);
                if (inserted_)
                {
                        inserted_ = false;
                        send_event(typename VolumeEvent<N>::Erase(id_));
                }
        }

        bool visible() const
        {
                std::shared_lock lock(mutex_);
                return visible_;
        }

        void set_visible(bool visible)
        {
                std::unique_lock lock(mutex_);
                if (visible_ == visible)
                {
                        return;
                }
                visible_ = visible;
                send_event(typename VolumeEvent<N>::Visibility(id_, visible));
        }
};

//

template <std::size_t N>
class Writing final
{
        VolumeObject<N>* object_;
        std::unique_lock<std::shared_mutex> lock_;

        Update::Flags updates_;

public:
        explicit Writing(VolumeObject<N>* object) : object_(object), lock_(object_->mutex_)
        {
        }

        Writing(const Writing&) = delete;
        Writing& operator=(const Writing&) = delete;
        Writing(Writing&&) = delete;
        Writing& operator=(Writing&&) = delete;

        ~Writing()
        {
                if (updates_.none())
                {
                        return;
                }
                object_->versions_.add(updates_);
                if (object_->inserted_)
                {
                        object_->send_event(typename VolumeEvent<N>::Update(object_->weak_from_this()));
                }
        }

        const Volume<N>& volume() const
        {
                return object_->volume();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix();
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                updates_.set(Update::Matrices);
                object_->set_matrix(matrix);
        }

        float level_min() const
        {
                return object_->level_min();
        }

        float level_max() const
        {
                return object_->level_max();
        }

        void set_levels(float min, float max)
        {
                updates_.set(Update::Levels);
                object_->set_levels(min, max);
        }

        float volume_alpha_coefficient() const
        {
                return object_->volume_alpha_coefficient();
        }

        void set_volume_alpha_coefficient(float coefficient)
        {
                updates_.set(Update::VolumeAlphaCoefficient);
                object_->set_volume_alpha_coefficient(coefficient);
        }

        float isosurface_alpha() const
        {
                return object_->isosurface_alpha();
        }

        void set_isosurface_alpha(float alpha)
        {
                updates_.set(Update::IsosurfaceAlpha);
                object_->set_isosurface_alpha(alpha);
        }

        bool isosurface() const
        {
                return object_->isosurface();
        }

        void set_isosurface(bool enabled)
        {
                updates_.set(Update::Isovalue);
                object_->set_isosurface(enabled);
        }

        float isovalue() const
        {
                return object_->isovalue();
        }

        void set_isovalue(float value)
        {
                updates_.set(Update::Isovalue);
                object_->set_isovalue(value);
        }

        const color::Color& color() const
        {
                return object_->color();
        }

        void set_color(const color::Color& color)
        {
                updates_.set(Update::Color);
                object_->set_color(color);
        }

        float ambient() const
        {
                return object_->ambient();
        }

        void set_ambient(float ambient)
        {
                updates_.set(Update::Ambient);
                object_->set_ambient(ambient);
        }

        float metalness() const
        {
                return object_->metalness();
        }

        void set_metalness(float metalness)
        {
                updates_.set(Update::Metalness);
                object_->set_metalness(metalness);
        }

        float roughness() const
        {
                return object_->roughness();
        }

        void set_roughness(float roughness)
        {
                updates_.set(Update::Roughness);
                object_->set_roughness(roughness);
        }
};

template <std::size_t N>
class Reading final
{
        const VolumeObject<N>* object_;
        std::shared_lock<std::shared_mutex> lock_;

public:
        explicit Reading(const VolumeObject<N>& object) : object_(&object), lock_(object.mutex_)
        {
        }

        void updates(std::optional<int>* version, Update::Flags* updates) const
        {
                object_->updates(version, updates);
        }

        const Volume<N>& volume() const
        {
                return object_->volume();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix();
        }

        float level_min() const
        {
                return object_->level_min();
        }

        float level_max() const
        {
                return object_->level_max();
        }

        float volume_alpha_coefficient() const
        {
                return object_->volume_alpha_coefficient();
        }

        float isosurface_alpha() const
        {
                return object_->isosurface_alpha();
        }

        bool isosurface() const
        {
                return object_->isosurface();
        }

        float isovalue() const
        {
                return object_->isovalue();
        }

        const color::Color& color() const
        {
                return object_->color();
        }

        float ambient() const
        {
                return object_->ambient();
        }

        float metalness() const
        {
                return object_->metalness();
        }

        float roughness() const
        {
                return object_->roughness();
        }
};
}
