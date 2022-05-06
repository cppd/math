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

#include "load_obj.h"

#include "data_read.h"
#include "file_lines.h"
#include "load_mtl.h"
#include "mesh_facet.h"
#include "obj_facet.h"
#include "obj_read.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>

#include <filesystem>
#include <latch>
#include <map>
#include <set>
#include <string>
#include <variant>

namespace ns::mesh::file
{
namespace
{
template <std::size_t N>
constexpr int MAX_FACETS_PER_LINE = 1;
template <>
constexpr int MAX_FACETS_PER_LINE<3> = 5;

std::string obj_type_name(const std::size_t n)
{
        return "OBJ-" + to_string(n);
}

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

struct Counters final
{
        int vertex = 0;
        int texcoord = 0;
        int normal = 0;
        int facet = 0;

        void operator+=(const Counters& counters)
        {
                vertex += counters.vertex;
                texcoord += counters.texcoord;
                normal += counters.normal;
                facet += counters.facet;
        }
};

template <std::size_t N>
struct Vertex final
{
        Vector<N, float> v;
};

template <std::size_t N>
struct TextureVertex final
{
        Vector<N - 1, float> v;
};

template <std::size_t N>
struct Normal final
{
        Vector<N, float> v;
};

template <std::size_t N>
struct Face final
{
        std::array<typename Mesh<N>::Facet, MAX_FACETS_PER_LINE<N>> facets;
        int count;
};

struct UseMaterial final
{
        const char* second_b;
        const char* second_e;
};

struct MaterialLibrary final
{
        const char* second_b;
        const char* second_e;
};

template <std::size_t N>
using ObjLine = std::variant<Vertex<N>, TextureVertex<N>, Normal<N>, Face<N>, UseMaterial, MaterialLibrary>;

template <std::size_t N>
std::optional<ObjLine<N>> read_obj_line(
        const std::string_view first,
        const char* const second_b,
        const char* const second_e,
        Counters* const counters)
{
        if (first == "v")
        {
                Vertex<N> data;
                read_float(second_b, &data.v);
                ++counters->vertex;
                return data;
        }

        if (first == "vt")
        {
                TextureVertex<N> data;
                read_float_texture(second_b, &data.v);
                ++counters->texcoord;
                return data;
        }

        if (first == "vn")
        {
                Normal<N> data;
                read_float(second_b, &data.v);
                data.v.normalize();
                if (!is_finite(data.v))
                {
                        data.v = Vector<N, float>(0);
                }
                ++counters->normal;
                return data;
        }

        if (first == "f")
        {
                Face<N> data;
                read_facets<N>(second_b, second_e, &data.facets, &data.count);
                ++counters->facet;
                return data;
        }

        if (first == "usemtl")
        {
                UseMaterial data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }

        if (first == "mtllib")
        {
                MaterialLibrary data;
                data.second_b = second_b;
                data.second_e = second_e;
                return data;
        }

        return std::nullopt;
}

template <std::size_t N>
class Visitor final
{
        std::map<std::string, int>* const material_index_;
        std::vector<std::filesystem::path>* const library_names_;
        Mesh<N>* const mesh_;

        int mtl_index_ = -1;
        std::set<std::filesystem::path> unique_library_names_;

public:
        Visitor(std::map<std::string, int>* const material_index,
                std::vector<std::filesystem::path>* const library_names,
                Mesh<N>* const mesh)
                : material_index_(material_index),
                  library_names_(library_names),
                  mesh_(mesh)
        {
        }

        void operator()(const Vertex<N>& data)
        {
                mesh_->vertices.push_back(data.v);
        }

        void operator()(const TextureVertex<N>& data)
        {
                mesh_->texcoords.resize(mesh_->texcoords.size() + 1);
                mesh_->texcoords[mesh_->texcoords.size() - 1] = data.v;
        }

        void operator()(const Normal<N>& data)
        {
                mesh_->normals.push_back(data.v);
        }

        void operator()(Face<N> data)
        {
                for (int i = 0; i < data.count; ++i)
                {
                        correct_indices<N>(
                                &data.facets[i], mesh_->vertices.size(), mesh_->texcoords.size(),
                                mesh_->normals.size());

                        data.facets[i].material = mtl_index_;

                        mesh_->facets.push_back(std::move(data.facets[i]));
                }
        }

        void operator()(const UseMaterial& data)
        {
                const std::string name{read_name("material", data.second_b, data.second_e)};

                if (const auto iter = material_index_->find(name); iter != material_index_->end())
                {
                        mtl_index_ = iter->second;
                        return;
                }

                mesh_->materials.push_back({.name = name});
                mtl_index_ = mesh_->materials.size() - 1;
                material_index_->emplace(name, mtl_index_);
        }

        void operator()(const MaterialLibrary& data)
        {
                read_library_names(data.second_b, data.second_e, library_names_, &unique_library_names_);
        }
};

template <std::size_t N>
void read_obj_stage_one(
        const unsigned thread_num,
        const unsigned thread_count,
        std::vector<Counters>* const counters,
        std::vector<char>* const data_ptr,
        const std::vector<long long>& line_begin,
        std::vector<std::optional<ObjLine<N>>>* const obj_lines,
        ProgressRatio* const progress)
{
        ASSERT(counters->size() == thread_count);

        const long long line_count = line_begin.size();
        const double line_count_reciprocal = 1.0 / line_begin.size();

        for (long long line_num = thread_num; line_num < line_count; line_num += thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                const SplitLine split = split_line(data_ptr, line_begin, line_num);

                try
                {
                        (*obj_lines)[line_num] =
                                read_obj_line<N>(split.first, split.second_b, split.second_e, &(*counters)[thread_num]);
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(line_num) + ": " + std::string(split.first) + " "
                              + std::string(split.second_b, split.second_e) + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line_num) + ": " + std::string(split.first) + " "
                              + std::string(split.second_b, split.second_e) + "\n" + "Unknown error");
                }
        }
}

template <std::size_t N>
void read_obj_stage_two(
        const Counters& counters,
        const std::vector<std::optional<ObjLine<N>>>& obj_lines,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        mesh->vertices.reserve(counters.vertex);
        mesh->texcoords.reserve(counters.texcoord);
        mesh->normals.reserve(counters.normal);
        mesh->facets.reserve(counters.facet);

        const long long line_count = obj_lines.size();
        const double line_count_reciprocal = 1.0 / obj_lines.size();

        Visitor<N> visitor(material_index, library_names, mesh);

        for (long long line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                if (obj_lines[line_num])
                {
                        std::visit(visitor, *obj_lines[line_num]);
                }
        }
}

Counters sum_counters(const std::vector<Counters>& counters)
{
        Counters sum;
        for (const Counters& c : counters)
        {
                sum += c;
        }
        return sum;
}

template <std::size_t N>
void read_obj_thread(
        const unsigned thread_num,
        const unsigned thread_count,
        std::vector<Counters>* const counters,
        std::latch* const latch,
        std::atomic_bool* const error_found,
        std::vector<char>* const data_ptr,
        std::vector<long long>* const line_begin,
        std::vector<std::optional<ObjLine<N>>>* const obj_lines,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        try
        {
                read_obj_stage_one(thread_num, thread_count, counters, data_ptr, *line_begin, obj_lines, progress);
        }
        catch (...)
        {
                error_found->store(true);
                latch->arrive_and_wait();
                throw;
        }
        latch->arrive_and_wait();
        if (*error_found)
        {
                return;
        }

        if (thread_num != 0)
        {
                return;
        }

        line_begin->clear();
        line_begin->shrink_to_fit();

        read_obj_stage_two(sum_counters(*counters), *obj_lines, progress, material_index, library_names, mesh);
}

template <std::size_t N>
void read_obj(
        const std::filesystem::path& file_name,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        const unsigned thread_count = hardware_concurrency();

        std::vector<char> data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &data, &line_begin);

        std::vector<std::optional<ObjLine<N>>> obj_lines{line_begin.size()};
        std::latch latch{thread_count};
        std::atomic_bool error_found{false};
        std::vector<Counters> counters{thread_count};

        ThreadsWithCatch threads{thread_count};
        for (unsigned i = 0; i < thread_count; ++i)
        {
                threads.add(
                        [&, i]()
                        {
                                read_obj_thread(
                                        i, thread_count, &counters, &latch, &error_found, &data, &line_begin,
                                        &obj_lines, progress, material_index, library_names, mesh);
                        });
        }
        threads.join();
}

template <std::size_t N>
void read_libs(
        const std::filesystem::path& dir_name,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        const std::vector<std::filesystem::path>& library_names,
        Mesh<N>* const mesh)
{
        std::map<std::filesystem::path, int> image_index;

        for (std::size_t i = 0; (i < library_names.size()) && !material_index->empty(); ++i)
        {
                read_lib(dir_name, library_names[i], progress, material_index, &image_index, mesh);
        }

        if (!material_index->empty())
        {
                error("Materials not found in libraries: " + map_keys_to_string(*material_index));
        }

        mesh->materials.shrink_to_fit();
        mesh->images.shrink_to_fit();
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> read_obj_and_mtl(const std::filesystem::path& file_name, ProgressRatio* const progress)
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
std::unique_ptr<Mesh<N>> load_from_obj_file(const Path& file_name, ProgressRatio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = read_obj_and_mtl<N>(file_name, progress);

        LOG(obj_type_name(N) + " loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
}
