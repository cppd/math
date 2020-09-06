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

#include "model_events.h"

#include <src/com/variant.h>

namespace gui
{
ModelEvents::ModelEvents(std::unique_ptr<ModelTree>* model_tree, std::unique_ptr<view::View>* view)
        : m_thread_id(std::this_thread::get_id()), m_model_tree(*model_tree), m_view(*view)
{
        const auto f = [this]<size_t N>(Events<N>& events) {
                events.mesh_events = [this](mesh::MeshEvent<N>&& event) { on_mesh(std::move(event)); };
                events.volume_events = [this](volume::VolumeEvent<N>&& event) { on_volume(std::move(event)); };
                mesh::MeshObject<N>::set_events(&events.mesh_events);
                volume::VolumeObject<N>::set_events(&events.volume_events);
        };

        std::apply(
                [&f]<size_t... N>(Events<N> & ... events) { (f(events), ...); }, m_events);
}

ModelEvents::~ModelEvents()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        const auto f = []<size_t N>(const Events<N>&) {
                mesh::MeshObject<N>::set_events(nullptr);
                volume::VolumeObject<N>::set_events(nullptr);
        };

        std::apply(
                [&f]<size_t... N>(const Events<N>&... events) { (f(events), ...); }, m_events);
}

template <size_t N>
void ModelEvents::on_mesh(mesh::MeshEvent<N>&& event)
{
        const auto visitors = Visitors{
                [this](const typename mesh::MeshEvent<N>::Insert& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateMeshObject(v.object));
                        }
                        ASSERT(v.object);
                        if (m_model_tree)
                        {
                                m_model_tree->insert(v.object, v.parent_object_id);
                        }
                },
                [this](const typename mesh::MeshEvent<N>::Erase& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::DeleteObject(v.id));
                        }
                        if (m_model_tree)
                        {
                                m_model_tree->erase(v.id);
                        }
                },
                [this](const typename mesh::MeshEvent<N>::Update& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateMeshObject(v.object));
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
                        if (m_model_tree)
                        {
                                m_model_tree->update(id);
                        }
                },
                [this](const typename mesh::MeshEvent<N>::Visibility& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::ShowObject(v.id, v.visible));
                        }
                        if (m_model_tree)
                        {
                                m_model_tree->show(v.id, v.visible);
                        }
                }};

        std::visit(visitors, event.data());
}

template <size_t N>
void ModelEvents::on_volume(volume::VolumeEvent<N>&& event)
{
        const auto visitors = Visitors{
                [this](const typename volume::VolumeEvent<N>::Insert& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateVolumeObject(v.object));
                        }
                        ASSERT(v.object);
                        if (m_model_tree)
                        {
                                m_model_tree->insert(v.object, v.parent_object_id);
                        }
                },
                [this](const typename volume::VolumeEvent<N>::Erase& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::DeleteObject(v.id));
                        }
                        if (m_model_tree)
                        {
                                m_model_tree->erase(v.id);
                        }
                },
                [this](const typename volume::VolumeEvent<N>::Update& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateVolumeObject(v.object));
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
                        if (m_model_tree)
                        {
                                m_model_tree->update(id);
                        }
                },
                [this](const typename volume::VolumeEvent<N>::Visibility& v) {
                        if constexpr (N == 3)
                        {
                                ASSERT(m_view);
                                m_view->send(view::command::ShowObject(v.id, v.visible));
                        }
                        if (m_model_tree)
                        {
                                m_model_tree->show(v.id, v.visible);
                        }
                }};

        std::visit(visitors, event.data());
}
}
