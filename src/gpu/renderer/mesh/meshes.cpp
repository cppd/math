/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "meshes.h"

#include "object.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ns::gpu::renderer
{
void find_opaque_and_transparent_meshes(
        const std::vector<const MeshObject*>& meshes,
        std::vector<const MeshObject*>* const opaque_meshes,
        std::vector<const MeshObject*>* const transparent_meshes)
{
        const std::size_t transparent_count = std::count_if(
                meshes.cbegin(), meshes.cend(),
                [](const MeshObject* const mesh)
                {
                        return mesh->transparent();
                });

        opaque_meshes->clear();
        opaque_meshes->reserve(meshes.size() - transparent_count);

        transparent_meshes->clear();
        transparent_meshes->reserve(transparent_count);

        for (const MeshObject* const mesh : meshes)
        {
                if (mesh->transparent())
                {
                        transparent_meshes->push_back(mesh);
                }
                else
                {
                        opaque_meshes->push_back(mesh);
                }
        }
}
}
