/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "load_obj.h"

#include "lines.h"
#include "mesh_facet.h"

#include "obj/counters.h"
#include "obj/data_read.h"
#include "obj/line.h"
#include "obj/load_lib.h"
#include "obj/name.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/read.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/model/mesh.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ns::model::mesh::file
{
namespace
{
using FloatingPointType = float;

template <typename T>
std::string map_keys_to_string(const std::map<std::string, T>& m)
{
        std::string names;
        for (const auto& s : m)
        {
                if (!names.empty())
                {
                        names += ", ";
                }
                names += s.first;
        }
        return names;
}

template <std::size_t N, typename T>
void read_obj_stage_one(
        const unsigned thread_num,
        const unsigned thread_count,
        const Lines& lines,
        obj::Counters* const counters,
        std::vector<std::optional<obj::Line<N, T>>>* const obj_lines,
        progress::Ratio* const progress)
{
        const std::size_t count = lines.size();
        const double count_reciprocal = 1.0 / count;

        for (std::size_t i = thread_num; i < count; i += thread_count)
        {
                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(i * count_reciprocal);
                }

                const obj::Split split = obj::split_string(lines.c_str_view(i));

                try
                {
                        (*obj_lines)[i] = obj::read_line<N, T>(split.first, split.second_b, split.second_e, counters);
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(i) + ": " + std::string(split.first) + " "
                              + std::string(split.second_b, split.second_e) + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(i) + ": " + std::string(split.first) + " "
                              + std::string(split.second_b, split.second_e) + "\n" + "Unknown error");
                }
        }
}

template <std::size_t N, typename T>
void read_obj_stage_two(
        const obj::Counters& counters,
        const std::vector<std::optional<obj::Line<N, T>>>& obj_lines,
        progress::Ratio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        mesh->vertices.reserve(counters.vertex);
        mesh->texcoords.reserve(counters.texcoord);
        mesh->normals.reserve(counters.normal);
        mesh->facets.reserve(counters.facet);

        const std::size_t count = obj_lines.size();
        const double count_reciprocal = 1.0 / obj_lines.size();

        obj::LineProcess<N> visitor(material_index, library_names, mesh);

        for (std::size_t i = 0; i < count; ++i)
        {
                if ((i & 0xfff) == 0xfff)
                {
                        progress->set(i * count_reciprocal);
                }

                if (const auto& line = obj_lines[i])
                {
                        std::visit(visitor, *line);
                }
        }
}

template <std::size_t N>
void read_obj(
        const std::filesystem::path& file_name,
        progress::Ratio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        const Lines lines(read_file(file_name));

        std::vector<std::optional<obj::Line<N, FloatingPointType>>> obj_lines{lines.size()};

        const unsigned thread_count = std::min(lines.size(), static_cast<std::size_t>(hardware_concurrency()));
        std::vector<obj::Counters> counters{thread_count};

        Threads threads{thread_count};
        for (unsigned thread = 0; thread < thread_count; ++thread)
        {
                threads.add(
                        [&, thread]()
                        {
                                read_obj_stage_one(
                                        thread, thread_count, lines, &counters[thread], &obj_lines, progress);
                        });
        }
        threads.join();

        read_obj_stage_two(sum_counters(counters), obj_lines, progress, material_index, library_names, mesh);
}

template <std::size_t N>
void read_libs(
        const std::filesystem::path& dir_name,
        progress::Ratio* const progress,
        std::map<std::string, int>* const material_index,
        const std::vector<std::filesystem::path>& library_names,
        Mesh<N>* const mesh)
{
        std::map<std::filesystem::path, int> image_index;

        for (std::size_t i = 0; (i < library_names.size()) && !material_index->empty(); ++i)
        {
                obj::read_lib(dir_name, library_names[i], progress, material_index, &image_index, mesh);
        }

        if (!material_index->empty())
        {
                error("Materials not found in libraries: " + map_keys_to_string(*material_index));
        }

        mesh->materials.shrink_to_fit();
        mesh->images.shrink_to_fit();
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> read_obj_and_mtl(const std::filesystem::path& file_name, progress::Ratio* const progress)
{
        progress->set_undefined();

        std::map<std::string, int> material_index;
        std::vector<std::filesystem::path> library_names;

        Mesh<N> mesh;

        read_obj(file_name, progress, &material_index, &library_names, &mesh);

        if (mesh.facets.empty())
        {
                error("No facets found in OBJ file");
        }

        check_and_correct_mesh_facets(&mesh);
        set_center_and_length(&mesh);

        read_libs(file_name.parent_path(), progress, &material_index, library_names, &mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_obj_file(const Path& file_name, progress::Ratio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = read_obj_and_mtl<N>(file_name, progress);

        LOG(obj::obj_name(N) + " loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

#define TEMPLATE(N) \
        template std::unique_ptr<Mesh<(N)>> load_from_obj_file(const std::filesystem::path&, progress::Ratio*);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
