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

#include "model_events.h"

#include <src/com/error.h>

namespace ns::gui
{
namespace
{
template <std::size_t N>
class Visitor final
{
        ModelTreeEvents* tree_;
        view::View* view_;

public:
        Visitor(ModelTreeEvents* const tree, view::View* const view) : tree_(tree), view_(view)
        {
        }

        //

        void operator()(const mesh::event::Insert<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::UpdateMeshObject(event.object));
                }
                ASSERT(event.object);
                tree_->insert(event.object, event.parent_object_id);
        }

        void operator()(const mesh::event::Erase<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::DeleteObject(event.id));
                }
                tree_->erase(event.id);
        }

        void operator()(const mesh::event::Update<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::UpdateMeshObject(event.object));
                }
                ObjectId id;
                {
                        auto ptr = event.object.lock();
                        if (!ptr)
                        {
                                return;
                        }
                        id = ptr->id();
                }
                tree_->update(id);
        }

        void operator()(const mesh::event::Visibility<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::ShowObject(event.id, event.visible));
                }
                tree_->show(event.id, event.visible);
        }

        //

        void operator()(const volume::event::Insert<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::UpdateVolumeObject(event.object));
                }
                ASSERT(event.object);
                tree_->insert(event.object, event.parent_object_id);
        }

        void operator()(const volume::event::Erase<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::DeleteObject(event.id));
                }
                tree_->erase(event.id);
        }

        void operator()(const volume::event::Update<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::UpdateVolumeObject(event.object));
                }
                ObjectId id;
                {
                        auto ptr = event.object.lock();
                        if (!ptr)
                        {
                                return;
                        }
                        id = ptr->id();
                }
                tree_->update(id);
        }

        void operator()(const volume::event::Visibility<N>& event) const
        {
                if constexpr (N == 3)
                {
                        view_->send(view::command::ShowObject(event.id, event.visible));
                }
                tree_->show(event.id, event.visible);
        }
};
}

template <std::size_t N>
void ModelEvents::Events<N>::send(mesh::MeshEvent<N>&& event) const
{
        std::visit(Visitor<N>(tree, view), event);
}

template <std::size_t N>
void ModelEvents::Events<N>::send(volume::VolumeEvent<N>&& event) const
{
        std::visit(Visitor<N>(tree, view), event);
}

ModelEvents::ModelEvents(ModelTreeEvents* const tree, view::View* const view)
{
        ASSERT(tree);
        ASSERT(view);

        const auto f = [&]<std::size_t N>(Events<N>& events)
        {
                events.tree = tree;
                events.view = view;
                mesh::MeshObject<N>::set_events(&events);
                volume::VolumeObject<N>::set_events(&events);
        };

        std::apply(
                [&f](auto&... events)
                {
                        (f(events), ...);
                },
                events_);
}

ModelEvents::~ModelEvents()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const auto f = []<std::size_t N>(const Events<N>&)
        {
                mesh::MeshObject<N>::set_events(nullptr);
                volume::VolumeObject<N>::set_events(nullptr);
        };

        std::apply(
                [&f](const auto&... events)
                {
                        (f(events), ...);
                },
                events_);
}
}
