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
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <variant>

namespace ns::mesh
{
template <std::size_t N>
class MeshObject;

namespace event
{
template <std::size_t N>
struct Insert final
{
        std::shared_ptr<MeshObject<N>> object;
        std::optional<ObjectId> parent_object_id;
        Insert(std::shared_ptr<MeshObject<N>>&& object, const std::optional<ObjectId>& parent_object_id)
                : object(std::move(object)), parent_object_id(parent_object_id)
        {
        }
};

template <std::size_t N>
struct Erase final
{
        ObjectId id;
        explicit Erase(const ObjectId id) : id(id)
        {
        }
};

template <std::size_t N>
struct Update final
{
        std::weak_ptr<MeshObject<N>> object;
        explicit Update(std::weak_ptr<MeshObject<N>>&& object) : object(std::move(object))
        {
        }
};

template <std::size_t N>
struct Visibility final
{
        ObjectId id;
        bool visible;
        Visibility(const ObjectId id, const bool visible) : id(id), visible(visible)
        {
        }
};
}

template <std::size_t N>
using MeshEvent = std::variant<event::Erase<N>, event::Insert<N>, event::Update<N>, event::Visibility<N>>;

template <std::size_t N>
class MeshEvents
{
protected:
        ~MeshEvents() = default;

public:
        virtual void send(MeshEvent<N>&&) const = 0;
};

enum Update
{
        UPDATE_MESH,
        UPDATE_ALPHA,
        UPDATE_MATRIX,
        UPDATE_COLOR,
        UPDATE_AMBIENT,
        UPDATE_METALNESS,
        UPDATE_ROUGHNESS
};
using Updates = std::bitset<UPDATE_ROUGHNESS + 1>;

template <std::size_t N>
class MeshObject final : public std::enable_shared_from_this<MeshObject<N>>
{
        template <std::size_t>
        friend class Writing;

        template <std::size_t>
        friend class Reading;

        //

        class DefaultEvents final : public MeshEvents<N>
        {
                void send(MeshEvent<N>&&) const override
                {
                }
        };
        static constexpr DefaultEvents DEFAULT_EVENTS{};

        inline static const MeshEvents<N>* events_ = &DEFAULT_EVENTS;

        //

        std::unique_ptr<const Mesh<N>> mesh_;
        Matrix<N + 1, N + 1, double> matrix_;
        std::string name_;
        ObjectId id_;

        float alpha_ = 1;

        color::Color color_{RGB8(220, 255, 220)};
        float ambient_ = 0.2;
        float metalness_ = 0.05;
        float roughness_ = 0.3;

        bool visible_ = false;
        bool inserted_ = false;

        mutable std::shared_mutex mutex_;

        Versions<Updates().size()> versions_;

        void send_event(MeshEvent<N>&& event) noexcept
        {
                try
                {
                        events_->send(std::move(event));
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
                return *mesh_;
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return matrix_;
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                matrix_ = matrix;
        }

        float alpha() const
        {
                return alpha_;
        }

        void set_alpha(const float alpha)
        {
                alpha_ = alpha;
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

        void set_ambient(const float ambient)
        {
                ambient_ = ambient;
        }

        float metalness() const
        {
                return metalness_;
        }

        void set_metalness(const float metalness)
        {
                metalness_ = metalness;
        }

        float roughness() const
        {
                return roughness_;
        }

        void set_roughness(const float roughness)
        {
                roughness_ = roughness;
        }

        Updates updates(std::optional<int>* const version) const
        {
                return versions_.updates(version);
        }

public:
        static void set_events(const MeshEvents<N>* const events)
        {
                if (events)
                {
                        ASSERT(events_ == &DEFAULT_EVENTS);
                        events_ = events;
                        return;
                }
                events_ = &DEFAULT_EVENTS;
        }

        MeshObject(std::unique_ptr<const Mesh<N>>&& mesh, const Matrix<N + 1, N + 1, double>& matrix, std::string name)
                : mesh_(std::move(mesh)), matrix_(matrix), name_(std::move(name))
        {
                ASSERT(mesh_);
        }

        MeshObject(const MeshObject&) = delete;
        MeshObject& operator=(const MeshObject&) = delete;
        MeshObject(MeshObject&&) = delete;
        MeshObject& operator=(MeshObject&&) = delete;

        ~MeshObject()
        {
                if (inserted_)
                {
                        send_event(event::Erase<N>(id_));
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

        bool visible() const
        {
                std::shared_lock lock(mutex_);
                return visible_;
        }

        void set_visible(const bool visible)
        {
                std::unique_lock lock(mutex_);
                if (visible_ == visible)
                {
                        return;
                }
                visible_ = visible;
                if (inserted_)
                {
                        send_event(event::Visibility<N>(id_, visible));
                }
        }
};

//

template <std::size_t N>
class Writing final
{
        MeshObject<N>* object_;
        std::unique_lock<std::shared_mutex> lock_;

        Updates updates_;

public:
        explicit Writing(MeshObject<N>* object) : object_(object), lock_(object_->mutex_)
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
                        error_fatal(std::string("Error in mesh writing destructor: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Error in mesh writing destructor");
                }
        }

        const std::string& name() const
        {
                return object_->name();
        }

        const ObjectId& id() const
        {
                return object_->id();
        }

        const Mesh<N>& mesh() const
        {
                return object_->mesh();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix();
        }

        void set_matrix(const Matrix<N + 1, N + 1, double>& matrix)
        {
                updates_.set(UPDATE_MATRIX);
                object_->set_matrix(matrix);
        }

        float alpha() const
        {
                return object_->alpha();
        }

        void set_alpha(const float alpha)
        {
                updates_.set(UPDATE_ALPHA);
                object_->set_alpha(alpha);
        }

        const color::Color& color() const
        {
                return object_->color();
        }

        void set_color(const color::Color& color)
        {
                updates_.set(UPDATE_COLOR);
                object_->set_color(color);
        }

        float ambient() const
        {
                return object_->ambient();
        }

        void set_ambient(const float ambient)
        {
                updates_.set(UPDATE_AMBIENT);
                object_->set_ambient(ambient);
        }

        float metalness() const
        {
                return object_->metalness();
        }

        void set_metalness(const float metalness)
        {
                updates_.set(UPDATE_METALNESS);
                object_->set_metalness(metalness);
        }

        float roughness() const
        {
                return object_->roughness();
        }

        void set_roughness(const float roughness)
        {
                updates_.set(UPDATE_ROUGHNESS);
                object_->set_roughness(roughness);
        }
};

template <std::size_t N>
class Reading final
{
        const MeshObject<N>* object_;
        std::shared_lock<std::shared_mutex> lock_;

public:
        explicit Reading(const MeshObject<N>& object) : object_(&object), lock_(object.mutex_)
        {
        }

        Updates updates(std::optional<int>* const version) const
        {
                return object_->updates(version);
        }

        const std::string& name() const
        {
                return object_->name();
        }

        const ObjectId& id() const
        {
                return object_->id();
        }

        const Mesh<N>& mesh() const
        {
                return object_->mesh();
        }

        const Matrix<N + 1, N + 1, double>& matrix() const
        {
                return object_->matrix();
        }

        float alpha() const
        {
                return object_->alpha();
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
