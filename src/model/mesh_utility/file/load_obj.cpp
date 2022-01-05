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

enum class ObjLineType
{
        V,
        VT,
        VN,
        F,
        USEMTL,
        MTLLIB,
        NONE,
        NOT_SUPPORTED
};

template <std::size_t N>
struct ObjLine
{
        ObjLineType type;
        long long second_b;
        long long second_e;
        std::array<typename Mesh<N>::Facet, MAX_FACETS_PER_LINE<N>> facets;
        int facet_count;
        Vector<N, float> v;
};

struct Counters
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
void read_obj_line(
        std::vector<Counters>* const counters,
        const unsigned thread_num,
        const char* const first,
        const std::vector<char>& data,
        ObjLine<N>* const lp)
{
        if (str_equal(first, "v"))
        {
                lp->type = ObjLineType::V;
                Vector<N, float> v;
                read_float(&data[lp->second_b], &v);
                lp->v = v;

                ++((*counters)[thread_num].vertex);
        }
        else if (str_equal(first, "vt"))
        {
                lp->type = ObjLineType::VT;
                Vector<N - 1, float> v;
                read_float_texture(&data[lp->second_b], &v);
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        lp->v[i] = v[i];
                }

                ++((*counters)[thread_num].texcoord);
        }
        else if (str_equal(first, "vn"))
        {
                lp->type = ObjLineType::VN;
                Vector<N, float> v;
                read_float(&data[lp->second_b], &v);
                lp->v = v.normalized();
                if (!is_finite(lp->v))
                {
                        lp->v = Vector<N, float>(0);
                }

                ++((*counters)[thread_num].normal);
        }
        else if (str_equal(first, "f"))
        {
                lp->type = ObjLineType::F;
                read_facets<N>(data, lp->second_b, lp->second_e, &lp->facets, &lp->facet_count);

                ++((*counters)[thread_num].facet);
        }
        else if (str_equal(first, "usemtl"))
        {
                lp->type = ObjLineType::USEMTL;
        }
        else if (str_equal(first, "mtllib"))
        {
                lp->type = ObjLineType::MTLLIB;
        }
        else if (!*first)
        {
                lp->type = ObjLineType::NONE;
        }
        else
        {
                lp->type = ObjLineType::NOT_SUPPORTED;
        }
}

template <std::size_t N>
void read_obj_stage_one(
        const unsigned thread_num,
        const unsigned thread_count,
        std::vector<Counters>* const counters,
        std::vector<char>* const data_ptr,
        std::vector<long long>* const line_begin,
        std::vector<ObjLine<N>>* const line_prop,
        ProgressRatio* const progress)
{
        ASSERT(counters->size() == thread_count);

        const long long line_count = line_begin->size();
        const double line_count_reciprocal = 1.0 / line_begin->size();

        for (long long line_num = thread_num; line_num < line_count; line_num += thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                ObjLine<N> obj_line;

                const char* first;
                const char* second;

                split_line(data_ptr, *line_begin, line_num, &first, &second, &obj_line.second_b, &obj_line.second_e);

                try
                {
                        read_obj_line(counters, thread_num, first, *data_ptr, &obj_line);
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(line_num) + ": " + first + " " + second + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line_num) + ": " + first + " " + second + "\n" + "Unknown error");
                }

                (*line_prop)[line_num] = obj_line;
        }
}

template <std::size_t N>
void read_obj_stage_two(
        const Counters& counters,
        std::vector<char>* const data_ptr,
        std::vector<ObjLine<N>>* const line_prop,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        mesh->vertices.reserve(counters.vertex);
        mesh->texcoords.reserve(counters.texcoord);
        mesh->normals.reserve(counters.normal);
        mesh->facets.reserve(counters.facet);

        const std::vector<char>& data = *data_ptr;
        const long long line_count = line_prop->size();
        const double line_count_reciprocal = 1.0 / line_prop->size();

        int mtl_index = -1;
        std::set<std::filesystem::path> unique_library_names;

        for (long long line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                ObjLine<N>& lp = (*line_prop)[line_num];

                switch (lp.type)
                {
                case ObjLineType::V:
                        mesh->vertices.push_back(lp.v);
                        break;
                case ObjLineType::VT:
                {
                        mesh->texcoords.resize(mesh->texcoords.size() + 1);
                        Vector<N - 1, float>& new_vector = mesh->texcoords[mesh->texcoords.size() - 1];
                        for (unsigned i = 0; i < N - 1; ++i)
                        {
                                new_vector[i] = lp.v[i];
                        }
                        break;
                }
                case ObjLineType::VN:
                        mesh->normals.push_back(lp.v);
                        break;
                case ObjLineType::F:
                        for (int i = 0; i < lp.facet_count; ++i)
                        {
                                lp.facets[i].material = mtl_index;
                                correct_indices<N>(
                                        &lp.facets[i], mesh->vertices.size(), mesh->texcoords.size(),
                                        mesh->normals.size());
                                mesh->facets.push_back(std::move(lp.facets[i]));
                        }
                        break;
                case ObjLineType::USEMTL:
                {
                        std::string mtl_name;
                        read_name("material", data, lp.second_b, lp.second_e, &mtl_name);
                        const auto iter = material_index->find(mtl_name);
                        if (iter != material_index->end())
                        {
                                mtl_index = iter->second;
                        }
                        else
                        {
                                typename Mesh<N>::Material mtl;
                                mtl.name = mtl_name;
                                mesh->materials.push_back(std::move(mtl));
                                material_index->emplace(std::move(mtl_name), mesh->materials.size() - 1);
                                mtl_index = mesh->materials.size() - 1;
                        }
                        break;
                }
                case ObjLineType::MTLLIB:
                        read_library_names(data, lp.second_b, lp.second_e, library_names, &unique_library_names);
                        break;
                case ObjLineType::NONE:
                case ObjLineType::NOT_SUPPORTED:
                        break;
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
        std::vector<ObjLine<N>>* const line_prop,
        ProgressRatio* const progress,
        std::map<std::string, int>* const material_index,
        std::vector<std::filesystem::path>* const library_names,
        Mesh<N>* const mesh)
{
        try
        {
                read_obj_stage_one(thread_num, thread_count, counters, data_ptr, line_begin, line_prop, progress);
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

        read_obj_stage_two(sum_counters(*counters), data_ptr, line_prop, progress, material_index, library_names, mesh);
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

        std::vector<ObjLine<N>> line_prop{line_begin.size()};
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
                                        &line_prop, progress, material_index, library_names, mesh);
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
        std::map<std::string, int> image_index;

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
