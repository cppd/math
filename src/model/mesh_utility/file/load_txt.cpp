/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "load_txt.h"

#include "data_read.h"
#include "lines.h"
#include "mesh_facet.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/read.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>

#include <filesystem>

namespace ns::model::mesh::file
{
namespace
{
// x0 x1 x2 x3 ...
// x0 x1 x2 x3 ...
template <std::size_t N>
void read_points_thread(
        const unsigned thread_num,
        const unsigned thread_count,
        const Lines& lines,
        std::vector<Vector<N, float>>* const vertices,
        ProgressRatio* const progress)
{
        const std::size_t line_count = lines.size();
        const double line_count_reciprocal = 1.0 / line_count;

        for (std::size_t line = thread_num; line < line_count; line += thread_count)
        {
                if ((line & 0xfff) == 0xfff)
                {
                        progress->set(line * line_count_reciprocal);
                }

                const char* const str = lines.c_str(line);

                try
                {
                        read(str, &(*vertices)[line]);
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(line) + ": " + str + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line) + ": " + str + "\n" + "Unknown error");
                }
        }
}

template <std::size_t N>
void read_points(
        std::vector<Vector<N, float>>* const vertices,
        const std::filesystem::path& file_name,
        ProgressRatio* const progress)
{
        const Lines lines(read_file(file_name));

        vertices->resize(lines.size());

        const unsigned thread_count = std::min(lines.size(), static_cast<std::size_t>(hardware_concurrency()));

        Threads threads{thread_count};
        for (unsigned thread = 0; thread < thread_count; ++thread)
        {
                threads.add(
                        [&, thread]()
                        {
                                read_points_thread(thread, thread_count, lines, vertices, progress);
                        });
        }
        threads.join();
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> read_text(const std::filesystem::path& file_name, ProgressRatio* const progress)
{
        progress->set_undefined();

        auto mesh = std::make_unique<Mesh<N>>();

        read_points(&mesh->vertices, file_name, progress);

        if (mesh->vertices.empty())
        {
                error("No vertices found in TXT file");
        }

        mesh->points.resize(mesh->vertices.size());
        for (std::size_t i = 0; i < mesh->points.size(); ++i)
        {
                mesh->points[i].vertex = i;
        }

        check_and_correct_mesh_facets(mesh.get());
        set_center_and_length(mesh.get());

        return mesh;
}
}

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_txt_file(const Path& file_name, ProgressRatio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = read_text<N>(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_txt_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load_from_txt_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load_from_txt_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load_from_txt_file(const std::filesystem::path&, ProgressRatio*);
}
