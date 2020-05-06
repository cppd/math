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

#include "load.h"

#include "mesh.h"
#include "volume.h"

#include <src/com/error.h>
#include <src/model/mesh_utility.h>
#include <src/storage/options.h>

namespace process
{
namespace
{
[[noreturn]] void dimension_error(unsigned dimension)
{
        std::string s;
        std::apply(
                [&]<size_t... N>(const storage::Dimension<N>...) {
                        (([&]() {
                                 if (!s.empty())
                                 {
                                         s += ", ";
                                 }
                                 s += to_string(N);
                         }()),
                         ...);
                },
                storage::DIMENSIONS);
        error("Dimension " + to_string(dimension) + " is not supported, supported dimensions are " + s + ".");
}

template <typename T>
void apply_for_dimension(size_t dimension, const T& f)
{
        bool found = std::apply(
                [&]<size_t... N>(const storage::Dimension<N>&...) {
                        return ([&]() {
                                if (N == dimension)
                                {
                                        f(storage::Dimension<N>());
                                        return true;
                                }
                                return false;
                        }() || ...);
                },
                storage::DIMENSIONS);

        if (!found)
        {
                dimension_error(dimension);
        }
}
}

void load_from_file(
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        ProgressRatioList* progress_list,
        const std::string& file_name,
        double object_size,
        const vec3& object_position,
        double rho,
        double alpha,
        const std::function<void()>& load_event)
{
        unsigned dimension = mesh::file_dimension(file_name);

        apply_for_dimension(dimension, [&]<size_t N>(const storage::Dimension<N>&) {
                std::unique_ptr<const mesh::Mesh<N>> mesh;

                {
                        ProgressRatio progress(progress_list);
                        progress.set_text("Loading file: %p%");
                        mesh = mesh::load<N>(file_name, &progress);
                }

                load_event();

                compute(progress_list, build_convex_hull, build_cocone, build_bound_cocone, build_mst, std::move(mesh),
                        "Model", object_size, object_position, rho, alpha);
        });
}

void load_from_mesh_repository(
        bool build_convex_hull,
        bool build_cocone,
        bool build_bound_cocone,
        bool build_mst,
        ProgressRatioList* progress_list,
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        double rho,
        double alpha,
        int point_count,
        const std::function<void()>& load_event,
        const storage::MultiRepository& repository)
{
        apply_for_dimension(dimension, [&]<size_t N>(const storage::Dimension<N>&) {
                std::unique_ptr<const mesh::Mesh<N>> mesh =
                        repository.repository<N>().meshes().object(object_name, point_count);

                load_event();

                compute(progress_list, build_convex_hull, build_cocone, build_bound_cocone, build_mst, std::move(mesh),
                        object_name, object_size, object_position, rho, alpha);
        });
}

void load_from_volume_repository(
        int dimension,
        const std::string& object_name,
        double object_size,
        const vec3& object_position,
        int image_size,
        const storage::MultiRepository& repository)
{
        apply_for_dimension(dimension, [&]<size_t N>(const storage::Dimension<N>&) {
                std::unique_ptr<const volume::Volume<N>> volume =
                        repository.repository<N>().volumes().object(object_name, image_size);

                compute(std::move(volume), object_name, object_size, object_position);
        });
}
}
