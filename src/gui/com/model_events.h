/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/sequence.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/settings/dimensions.h>
#include <src/view/view.h>

#include <cstddef>
#include <thread>
#include <tuple>

namespace ns::gui
{
class ModelEvents final
{
        template <std::size_t N>
        class Events final : public model::mesh::MeshEvents<N>, public model::volume::VolumeEvents<N>
        {
                ModelTreeEvents* tree_;
                view::View* view_;

                void send(model::mesh::MeshEvent<N>&& event) const override;
                void send(model::volume::VolumeEvent<N>&& event) const override;

        public:
                Events()
                {
                }

                void set(ModelTreeEvents* const tree, view::View* const view)
                {
                        tree_ = tree;
                        view_ = view;
                }
        };

        const std::thread::id thread_id_ = std::this_thread::get_id();
        Sequence<settings::Dimensions, std::tuple, Events> events_;

public:
        ModelEvents(ModelTreeEvents* tree, view::View* view);
        ~ModelEvents();

        ModelEvents(const ModelEvents&) = delete;
        ModelEvents(ModelEvents&&) = delete;
        ModelEvents& operator=(const ModelEvents&) = delete;
        ModelEvents& operator=(ModelEvents&&) = delete;
};
}
