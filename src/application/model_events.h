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

#include <src/com/sequence.h>
#include <src/gui/model_tree.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/settings/dimensions.h>
#include <src/view/interface.h>

#include <thread>
#include <tuple>

namespace ns::application
{
class ModelEvents final
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        template <std::size_t N>
        struct Events final : public mesh::MeshEvents<N>, public volume::VolumeEvents<N>
        {
                gui::ModelTreeEvents* tree;
                view::View* view;

                void send(mesh::MeshEvent<N>&& event) const override;
                void send(volume::VolumeEvent<N>&& event) const override;
        };
        Sequence<settings::Dimensions, std::tuple, Events> events_;

public:
        ModelEvents(gui::ModelTreeEvents* tree, view::View* view);
        ~ModelEvents();

        ModelEvents(const ModelEvents&) = delete;
        ModelEvents(ModelEvents&&) = delete;
        ModelEvents& operator=(const ModelEvents&) = delete;
        ModelEvents& operator=(ModelEvents&&) = delete;
};
}
