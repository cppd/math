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

#include "model_tree.h"

#include "../application/thread_switch.h"

#include <src/com/sequence.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/settings/dimensions.h>
#include <src/storage/storage.h>
#include <src/view/interface.h>

#include <functional>
#include <thread>
#include <tuple>

namespace gui
{
class ModelEvents final
{
        const std::thread::id m_thread_id;

        application::ThreadSwitch m_thread_switch;

        template <size_t N>
        struct Events
        {
                std::function<void(mesh::MeshEvent<N>&&)> mesh_events;
                std::function<void(volume::VolumeEvent<N>&&)> volume_events;
        };
        Sequence<settings::Dimensions, std::tuple, Events> m_events;

        std::unique_ptr<ModelTree>& m_model_tree;
        std::unique_ptr<view::View>& m_view;

        template <size_t N>
        void event_from_mesh([[maybe_unused]] const mesh::MeshEvent<N>& event);
        template <size_t N>
        void event_from_mesh_ui_thread(const mesh::MeshEvent<N>& event);
        template <size_t N>
        void event_from_volume([[maybe_unused]] const volume::VolumeEvent<N>& event);
        template <size_t N>
        void event_from_volume_ui_thread(const volume::VolumeEvent<N>& event);

public:
        ModelEvents(std::unique_ptr<ModelTree>* model_tree, std::unique_ptr<view::View>* view);
        ~ModelEvents();

        ModelEvents(const ModelEvents&) = delete;
        ModelEvents& operator=(const ModelEvents&) = delete;
};
}
