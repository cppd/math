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

#include "load_obj.h"

#include "data_read.h"
#include "file_lines.h"
#include "load_mtl.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/ascii.h>
#include <src/com/thread.h>
#include <src/com/type/name.h>

#include <filesystem>
#include <latch>
#include <map>
#include <set>
#include <string>
#include <thread>

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
                        names += ", " + s.first;
                }
                else
                {
                        names += s.first;
                }
        }
        return names;
}

// "x/x/x"
// "x//x"
// "x//"
// "x/x/"
// "x/x"
// "x"
template <typename T, std::size_t GROUP_SIZE, typename IndexType>
void read_digit_group(
        const T& line,
        const long long end,
        long long* const from,
        std::array<IndexType, GROUP_SIZE>* const group_indices)
{
        // vertex
        if (read_integer(line, end, from, &(*group_indices)[0]))
        {
                if ((*group_indices)[0] == 0)
                {
                        error("Zero facet index");
                }
        }
        else
        {
                error("Error read facet vertex first number");
        }

        // texture and normal
        for (int a = 1; a < static_cast<int>(group_indices->size()); ++a)
        {
                if (*from == end || ascii::is_space(line[*from]))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                if (line[*from] != '/')
                {
                        error(std::string("Error read facet number, expected '/', found '") + line[*from] + "'");
                }

                ++(*from);

                if (*from == end || ascii::is_space(line[*from]))
                {
                        (*group_indices)[a] = 0;
                        continue;
                }

                if (read_integer(line, end, from, &(*group_indices)[a]))
                {
                        if ((*group_indices)[a] == 0)
                        {
                                error("Zero facet index");
                        }
                }
                else
                {
                        (*group_indices)[a] = 0;
                }
        }
}

template <typename T, std::size_t MAX_GROUP_COUNT, std::size_t GROUP_SIZE, typename IndexType>
void read_digit_groups(
        const T& line,
        const long long begin,
        const long long end,
        std::array<std::array<IndexType, GROUP_SIZE>, MAX_GROUP_COUNT>* const group_ptr,
        int* const group_count)
{
        int group_index = -1;

        long long i = begin;

        while (true)
        {
                ++group_index;

                read(line, end, ascii::is_space, &i);

                if (i == end)
                {
                        *group_count = group_index;
                        return;
                }

                if (group_index >= static_cast<int>(group_ptr->size()))
                {
                        error("Found too many facet vertices " + to_string(group_index + 1)
                              + " (max supported = " + to_string(group_ptr->size()) + ")");
                }

                read_digit_group(line, end, &i, &(*group_ptr)[group_index]);
        }
}

template <typename T, std::size_t MAX_GROUP_COUNT>
void check_index_consistent(const std::array<std::array<T, 3>, MAX_GROUP_COUNT>& groups, const int group_count)
{
        // 0 means there is no index.
        // index order: facet, texture, normal.

        ASSERT(group_count <= static_cast<int>(groups.size()));

        int texture = 0;
        int normal = 0;

        for (int i = 0; i < group_count; ++i)
        {
                texture += (groups[i][1] != 0) ? 1 : 0;
                normal += (groups[i][2] != 0) ? 1 : 0;
        }

        if (!(texture == 0 || texture == group_count))
        {
                error("Inconsistent facet texture indices");
        }

        if (!(normal == 0 || normal == group_count))
        {
                error("Inconsistent facet normal indices");
        }
}

template <std::size_t N, typename T>
void read_facets(
        const T& data,
        const long long begin,
        const long long end,
        std::array<typename Mesh<N>::Facet, MAX_FACETS_PER_LINE<N>>* const facets,
        int* const facet_count)
{
        static_assert(N >= 3);

        constexpr int MAX_GROUP_COUNT = MAX_FACETS_PER_LINE<N> + N - 1;

        std::array<std::array<int, 3>, MAX_GROUP_COUNT> groups;

        int group_count;

        read_digit_groups(data, begin, end, &groups, &group_count);

        if (group_count < static_cast<int>(N))
        {
                error("Error facet vertex count " + to_string(group_count) + " (min = " + to_string(N) + ")");
        }

        check_index_consistent(groups, group_count);

        *facet_count = group_count - (N - 1);

        for (int i = 0; i < *facet_count; ++i)
        {
                (*facets)[i].has_texcoord = !(groups[0][1] == 0);
                (*facets)[i].has_normal = !(groups[0][2] == 0);

                (*facets)[i].vertices[0] = groups[0][0];
                (*facets)[i].texcoords[0] = groups[0][1];
                (*facets)[i].normals[0] = groups[0][2];

                for (unsigned n = 1; n < N; ++n)
                {
                        (*facets)[i].vertices[n] = groups[i + n][0];
                        (*facets)[i].texcoords[n] = groups[i + n][1];
                        (*facets)[i].normals[n] = groups[i + n][2];
                }
        }
}

template <std::size_t N, typename T>
void read_float_texture(const char* const str, Vector<N, T>* const v)
{
        T tmp;

        int n = read_float(str, v, &tmp).first;

        if (n != N && n != N + 1)
        {
                error("Error read " + to_string(N) + " or " + to_string(N + 1) + " floating points of " + type_name<T>()
                      + " type");
        }

        if (n == N + 1 && tmp != 0)
        {
                error(to_string(N + 1) + "-dimensional textures are not supported");
        }
}

template <typename T>
void read_library_names(
        const T& data,
        const long long begin,
        const long long end,
        std::vector<std::filesystem::path>* const v,
        std::set<std::filesystem::path>* const lib_unique_names)
{
        const long long size = end;
        bool found = false;
        long long i = begin;

        while (true)
        {
                read(data, size, ascii::is_space, &i);
                if (i == size)
                {
                        if (!found)
                        {
                                error("Library name not found");
                        }
                        return;
                }

                long long i2 = i;
                read(data, size, ascii::is_not_space, &i2);
                std::filesystem::path name = path_from_utf8(std::string(&data[i], i2 - i));
                i = i2;
                found = true;

                if (!lib_unique_names->contains(name))
                {
                        v->push_back(name);
                        lib_unique_names->insert(std::move(name));
                }
        }
}

template <std::size_t N>
bool facet_dimension_is_correct(const std::vector<Vector<N, float>>& vertices, const std::array<int, N>& indices)
{
        static_assert(N == 3);

        Vector<3, double> e0 = to_vector<double>(vertices[indices[1]] - vertices[indices[0]]);
        Vector<3, double> e1 = to_vector<double>(vertices[indices[2]] - vertices[indices[0]]);

        if (e0[1] * e1[2] - e0[2] * e1[1] != 0)
        {
                return true;
        }

        if (e0[0] * e1[2] - e0[2] * e1[0] != 0)
        {
                return true;
        }

        if (e0[0] * e1[1] - e0[1] * e1[0] != 0)
        {
                return true;
        }

        return false;
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
void check_facet_indices(
        const int vertex_count,
        const int texcoord_count,
        const int normal_count,
        const typename Mesh<N>::Facet& facet)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (facet.vertices[i] < 0 || facet.vertices[i] >= vertex_count)
                {
                        error("Vertex index " + to_string(facet.vertices[i]) + " is out of bounds [0, "
                              + to_string(vertex_count) + ")");
                }

                if (facet.has_texcoord)
                {
                        if (facet.texcoords[i] < 0 || facet.texcoords[i] >= texcoord_count)
                        {
                                error("Texture coordinate index " + to_string(facet.texcoords[i])
                                      + " is out of bounds [0, " + to_string(texcoord_count) + ")");
                        }
                }
                else
                {
                        if (facet.texcoords[i] != -1)
                        {
                                error("No texture but texture coordinate index is not set to -1");
                        }
                }

                if (facet.has_normal)
                {
                        if (facet.normals[i] < 0 || facet.normals[i] >= normal_count)
                        {
                                error("Normal index " + to_string(facet.normals[i]) + " is out of bounds [0, "
                                      + to_string(normal_count) + ")");
                        }
                }
                else
                {
                        if (facet.normals[i] != -1)
                        {
                                error("No normals but normal coordinate index is not set to -1");
                        }
                }
        }
}

template <std::size_t N>
void check_facet_indices(const Mesh<N>& mesh)
{
        const int vertex_count = mesh.vertices.size();
        const int texcoord_count = mesh.texcoords.size();
        const int normal_count = mesh.normals.size();

        for (const typename Mesh<N>::Facet& facet : mesh.facets)
        {
                check_facet_indices<N>(vertex_count, texcoord_count, normal_count, facet);
        }
}

template <std::size_t N>
bool remove_facets_with_incorrect_dimension([[maybe_unused]] Mesh<N>* mesh)
{
        if constexpr (N != 3)
        {
                return false;
        }
        else
        {
                std::vector<bool> wrong_facets(mesh->facets.size(), false);

                int wrong_facet_count = 0;

                for (std::size_t i = 0; i < mesh->facets.size(); ++i)
                {
                        if (!facet_dimension_is_correct(mesh->vertices, mesh->facets[i].vertices))
                        {
                                wrong_facets[i] = true;
                                ++wrong_facet_count;
                        }
                }

                if (wrong_facet_count == 0)
                {
                        return false;
                }

                std::vector<typename Mesh<N>::Facet> facets;
                facets.reserve(mesh->facets.size() - wrong_facet_count);

                for (std::size_t i = 0; i < mesh->facets.size(); ++i)
                {
                        if (!wrong_facets[i])
                        {
                                facets.push_back(mesh->facets[i]);
                        }
                }

                mesh->facets = std::move(facets);

                return true;
        }
}

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

// Positive OBJ indices indicate absolute vertex numbers.
// Negative OBJ indices indicate relative vertex numbers.
// Convert to absolute numbers starting at 0.
template <std::size_t N>
void correct_indices(
        typename Mesh<N>::Facet* const facet,
        const int vertices_size,
        const int texcoords_size,
        const int normals_size)
{
        for (unsigned i = 0; i < N; ++i)
        {
                int& v = facet->vertices[i];
                int& t = facet->texcoords[i];
                int& n = facet->normals[i];

                if (v == 0)
                {
                        error("Correct indices vertex index is zero");
                }

                v = v > 0 ? v - 1 : vertices_size + v;
                t = t > 0 ? t - 1 : (t < 0 ? texcoords_size + t : -1);
                n = n > 0 ? n - 1 : (n < 0 ? normals_size + n : -1);
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
                        auto iter = material_index->find(mtl_name);
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

        check_facet_indices(mesh);

        set_center_and_length(&mesh);

        if (remove_facets_with_incorrect_dimension(&mesh))
        {
                if (mesh.facets.empty())
                {
                        error("No " + to_string(N - 1) + "-facets found in " + obj_type_name(N) + " file");
                }
                set_center_and_length(&mesh);
        }

        read_libs(file_name.parent_path(), progress, &material_index, library_names, &mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_obj_file(const Path& file_name, ProgressRatio* const progress)
{
        Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = read_obj_and_mtl<N>(file_name, progress);

        LOG(obj_type_name(N) + " loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load_from_obj_file(const std::filesystem::path&, ProgressRatio*);
}
