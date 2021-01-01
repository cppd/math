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

#include "computing.h"

#include "mesh.h"

#include <src/gui/dialogs/bound_cocone.h>

namespace ns::process
{
std::function<void(ProgressRatioList*)> action_bound_cocone(const storage::MeshObjectConst& object)
{
        std::optional<gui::dialog::BoundCoconeParameters> parameters = gui::dialog::BoundCoconeParametersDialog::show();
        if (!parameters)
        {
                return nullptr;
        }

        return std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object)
                {
                        std::function<void(ProgressRatioList*)> f = [=](ProgressRatioList* progress_list)
                        {
                                compute(progress_list, false /*convex hull*/, false /*cocone*/, true /*bound cocone*/,
                                        false /*mst*/, *mesh_object, parameters->rho, parameters->alpha);
                        };
                        return f;
                },
                object);
}
}
