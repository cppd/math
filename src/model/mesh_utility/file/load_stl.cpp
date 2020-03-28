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

#include "load_stl.h"

#include "../position.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>

namespace mesh::file
{
namespace
{
template <size_t N>
std::unique_ptr<Mesh<N>> read_stl(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        Mesh<N> mesh;

        if (file_name.empty())
        {
                error("STL loading is not implemented");
        }

        set_center_and_length(&mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> load_from_stl_file(const std::string& /*file_name*/, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        std::unique_ptr<Mesh<N>> mesh = read_stl<N>("", progress);

        LOG("STL loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<4>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<5>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<6>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
}
