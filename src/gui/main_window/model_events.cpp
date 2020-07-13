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
ModelEvents::ModelEvents(
        std::unique_ptr<ModelTree>* model_tree,
        std::unique_ptr<view::View>* view,
        std::function<void(ObjectId)>&& on_mesh_update,
        std::function<void(ObjectId)>&& on_volume_update)
        : m_thread_id(std::this_thread::get_id()),
          m_model_tree(*model_tree),
          m_view(*view),
          m_on_mesh_update(std::move(on_mesh_update)),
          m_on_volume_update(std::move(on_volume_update))
{
        const auto f = [this]<size_t N>(Events<N>& model_events) {
                model_events.mesh_events = [this](mesh::MeshEvent<N>&& event) {
                        event_from_mesh(event);
                        m_thread_switch.run_in_object_thread(
                                [&, event = std::move(event)]() { event_from_mesh_ui_thread(event); });
                };

                model_events.volume_events = [this](volume::VolumeEvent<N>&& event) {
                        event_from_volume(event);
                        m_thread_switch.run_in_object_thread(
                                [&, event = std::move(event)]() { event_from_volume_ui_thread(event); });
                };

                mesh::MeshObject<N>::set_events(&model_events.mesh_events);

                volume::VolumeObject<N>::set_events(&model_events.volume_events);
        };

        std::apply(
                [&f]<size_t... N>(Events<N> & ... model_events) { (f(model_events), ...); }, m_events);
}

ModelEvents::~ModelEvents()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        const auto f = []<size_t N>(const Events<N>&) {
                mesh::MeshObject<N>::set_events(nullptr);

                volume::VolumeObject<N>::set_events(nullptr);
        };

        std::apply(
                [&f]<size_t... N>(const Events<N>&... model_events) { (f(model_events), ...); }, m_events);
}

template <size_t N>
void ModelEvents::event_from_mesh(const mesh::MeshEvent<N>& event)
{
        if constexpr (N == 3)
        {
                const auto visitors = Visitors{
                        [this](const typename mesh::MeshEvent<N>::Insert& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateMeshObject(v.object));
                        },
                        [this](const typename mesh::MeshEvent<N>::Update& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateMeshObject(v.object));
                        },
                        [this](const typename mesh::MeshEvent<N>::Delete& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::DeleteObject(v.id));
                        },
                        [this](const typename mesh::MeshEvent<N>::Visibility& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::ShowObject(v.id, v.visible));
                        }};

                std::visit(visitors, event.data());
        }
}

template <size_t N>
void ModelEvents::event_from_mesh_ui_thread(const mesh::MeshEvent<N>& event)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        const auto visitors = Visitors{
                [this](const typename mesh::MeshEvent<N>::Insert& v) {
                        if (v.object)
                        {
                                ASSERT(m_model_tree);
                                m_model_tree->insert_into_tree_and_storage(v.object, v.parent_object_id);
                        }
                },
                [this](const typename mesh::MeshEvent<N>::Update& v) {
                        if (v.object)
                        {
                                ASSERT(m_on_mesh_update);
                                m_on_mesh_update(v.object->id());
                        }
                },
                [this](const typename mesh::MeshEvent<N>::Delete& v) {
                        ASSERT(m_model_tree);
                        m_model_tree->erase_from_tree(v.id);
                },
                [this](const typename mesh::MeshEvent<N>::Visibility& v) {
                        ASSERT(m_model_tree);
                        m_model_tree->set_visible_in_tree(v.id, v.visible);
                }};

        std::visit(visitors, event.data());
}

template <size_t N>
void ModelEvents::event_from_volume(const volume::VolumeEvent<N>& event)
{
        if constexpr (N == 3)
        {
                const auto visitors = Visitors{
                        [this](const typename volume::VolumeEvent<N>::Insert& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateVolumeObject(v.object));
                        },
                        [this](const typename volume::VolumeEvent<N>::Update& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::UpdateVolumeObject(v.object));
                        },
                        [this](const typename volume::VolumeEvent<N>::Delete& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::DeleteObject(v.id));
                        },
                        [this](const typename volume::VolumeEvent<N>::Visibility& v) {
                                ASSERT(m_view);
                                m_view->send(view::command::ShowObject(v.id, v.visible));
                        }};

                std::visit(visitors, event.data());
        }
}

template <size_t N>
void ModelEvents::event_from_volume_ui_thread(const volume::VolumeEvent<N>& event)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        const auto visitors = Visitors{
                [this](const typename volume::VolumeEvent<N>::Insert& v) {
                        if (v.object)
                        {
                                ASSERT(m_model_tree);
                                m_model_tree->insert_into_tree_and_storage(v.object, v.parent_object_id);
                        }
                },
                [this](const typename volume::VolumeEvent<N>::Update& v) {
                        if (v.object)
                        {
                                ASSERT(m_on_volume_update);
                                m_on_volume_update(v.object->id());
                        }
                },
                [this](const typename volume::VolumeEvent<N>::Delete& v) {
                        ASSERT(m_model_tree);
                        m_model_tree->erase_from_tree(v.id);
                },
                [this](const typename volume::VolumeEvent<N>::Visibility& v) {
                        ASSERT(m_model_tree);
                        m_model_tree->set_visible_in_tree(v.id, v.visible);
                }};

        std::visit(visitors, event.data());
}
}
