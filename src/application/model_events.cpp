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

#include <src/com/variant.h>

namespace ns::application
{
void ModelEvents::set(Sequence<settings::Dimensions, std::tuple, Events>* all_events)
{
        ASSERT(all_events);

        const auto f = []<std::size_t N>(Events<N>& events)
        {
                events.saved_mesh_events = mesh::MeshObject<N>::set_events(&events.mesh_events);
                events.saved_volume_events = volume::VolumeObject<N>::set_events(&events.volume_events);
        };

        std::apply(
                [&f]<std::size_t... N>(Events<N> & ... events)
                {
                        (f(events), ...);
                },
                *all_events);
}

void ModelEvents::unset(const Sequence<settings::Dimensions, std::tuple, Events>& all_events)
{
        const auto f = []<std::size_t N>(const Events<N>& events)
        {
                const std::function<void(mesh::MeshEvent<N> &&)>* m =
                        mesh::MeshObject<N>::set_events(events.saved_mesh_events);

                const std::function<void(volume::VolumeEvent<N> &&)>* v =
                        volume::VolumeObject<N>::set_events(events.saved_volume_events);

                if (m != &events.mesh_events || v != &events.volume_events)
                {
                        ASSERT(false);
                }
        };

        std::apply(
                [&f]<std::size_t... N>(const Events<N>&... events)
                {
                        (f(events), ...);
                },
                all_events);
}

ModelEvents::ModelEvents(gui::ModelTreeEvents* tree, view::View* view)
{
        ASSERT(tree);
        ASSERT(view);

        const auto f = [&]<std::size_t N>(Events<N>& events)
        {
                auto mesh_visitors = Visitors{
                        [=](const typename mesh::MeshEvent<N>::Insert& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::UpdateMeshObject(v.object));
                                }
                                ASSERT(v.object);
                                tree->insert(v.object, v.parent_object_id);
                        },
                        [=](const typename mesh::MeshEvent<N>::Erase& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::DeleteObject(v.id));
                                }
                                tree->erase(v.id);
                        },
                        [=](const typename mesh::MeshEvent<N>::Update& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::UpdateMeshObject(v.object));
                                }
                                ObjectId id;
                                {
                                        auto ptr = v.object.lock();
                                        if (!ptr)
                                        {
                                                return;
                                        }
                                        id = ptr->id();
                                }
                                tree->update(id);
                        },
                        [=](const typename mesh::MeshEvent<N>::Visibility& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::ShowObject(v.id, v.visible));
                                }
                                tree->show(v.id, v.visible);
                        }};

                auto volume_visitors = Visitors{
                        [=](const typename volume::VolumeEvent<N>::Insert& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::UpdateVolumeObject(v.object));
                                }
                                ASSERT(v.object);
                                tree->insert(v.object, v.parent_object_id);
                        },
                        [=](const typename volume::VolumeEvent<N>::Erase& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::DeleteObject(v.id));
                                }
                                tree->erase(v.id);
                        },
                        [=](const typename volume::VolumeEvent<N>::Update& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::UpdateVolumeObject(v.object));
                                }
                                ObjectId id;
                                {
                                        auto ptr = v.object.lock();
                                        if (!ptr)
                                        {
                                                return;
                                        }
                                        id = ptr->id();
                                }
                                tree->update(id);
                        },
                        [=](const typename volume::VolumeEvent<N>::Visibility& v)
                        {
                                if constexpr (N == 3)
                                {
                                        view->send(view::command::ShowObject(v.id, v.visible));
                                }
                                tree->show(v.id, v.visible);
                        }};

                events.mesh_events = [mesh_visitors](mesh::MeshEvent<N>&& event)
                {
                        std::visit(mesh_visitors, event.data());
                };
                events.volume_events = [volume_visitors](volume::VolumeEvent<N>&& event)
                {
                        std::visit(volume_visitors, event.data());
                };
        };

        std::apply(
                [&f]<std::size_t... N>(Events<N> & ... events)
                {
                        (f(events), ...);
                },
                events_);

        set(&events_);
}

ModelEvents::ModelEvents()
{
        const auto f = []<std::size_t N>(Events<N>& events)
        {
                events.mesh_events = [](mesh::MeshEvent<N>&&) {};
                events.volume_events = [](volume::VolumeEvent<N>&&) {};
        };

        std::apply(
                [&f]<std::size_t... N>(Events<N> & ... events)
                {
                        (f(events), ...);
                },
                events_);

        set(&events_);
}

ModelEvents::~ModelEvents()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        unset(events_);
}
}
