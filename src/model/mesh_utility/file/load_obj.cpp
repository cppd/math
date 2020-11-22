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

#include "load_obj.h"

#include "data_read.h"
#include "file_lines.h"

#include "../position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/com/type/trait.h>
#include <src/image/file.h>
#include <src/image/flip.h>
#include <src/utility/file/path.h>
#include <src/utility/string/ascii.h>
#include <src/utility/string/str.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <thread>

namespace mesh::file
{
namespace
{
template <size_t N>
constexpr int MAX_FACETS_PER_LINE = 1;
template <>
constexpr int MAX_FACETS_PER_LINE<3> = 5;

constexpr const char* OBJ_v = "v";
constexpr const char* OBJ_vt = "vt";
constexpr const char* OBJ_vn = "vn";
constexpr const char* OBJ_f = "f";
constexpr const char* OBJ_usemtl = "usemtl";
constexpr const char* OBJ_mtllib = "mtllib";

constexpr const char* MTL_newmtl = "newmtl";
constexpr const char* MTL_Ka = "Ka";
constexpr const char* MTL_Kd = "Kd";
constexpr const char* MTL_Ks = "Ks";
constexpr const char* MTL_Ns = "Ns";
constexpr const char* MTL_map_Ka = "map_Ka";
constexpr const char* MTL_map_Kd = "map_Kd";
constexpr const char* MTL_map_Ks = "map_Ks";

constexpr bool is_number_sign(char c)
{
        return c == '#';
}

constexpr bool is_solidus(char c)
{
        return c == '/';
}

constexpr bool str_equal(const char* s1, const char* s2)
{
        while (*s1 == *s2 && *s1)
        {
                ++s1;
                ++s2;
        }
        return *s1 == *s2;
}

static_assert(
        str_equal("ab", "ab") && str_equal("", "") && !str_equal("", "ab") && !str_equal("ab", "")
        && !str_equal("ab", "ac") && !str_equal("ba", "ca") && !str_equal("a", "xyz"));

std::string obj_type_name(size_t N)
{
        return "OBJ-" + to_string(N);
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

template <typename T1, typename T2, typename T3>
bool check_range(const T1& v, const T2& min, const T3& max)
{
        return v >= min && v <= max;
}

bool check_color(const Color& v)
{
        return v.red() >= 0 && v.red() <= 1 && v.green() >= 0 && v.green() <= 1 && v.blue() >= 0 && v.blue() <= 1;
}

template <size_t N>
image::Image<N> read_image_from_file(const std::filesystem::path& file_name)
{
        if constexpr (N != 2)
        {
                error("Reading " + to_string(N - 1) + "-dimensional images for " + obj_type_name(N)
                      + " is not supported");
        }
        else
        {
                image::Image<2> obj_image;
                load_image_from_file_rgba(file_name, &obj_image);
                flip_image_vertically(&obj_image);
                return obj_image;
        }
}

template <size_t N>
void load_image(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& image_name,
        std::map<std::string, int>* image_index,
        std::vector<image::Image<N - 1>>* images,
        int* index)
{
        std::filesystem::path file_name = path_from_utf8(trim(generic_utf8_filename(image_name)));

        if (file_name.empty())
        {
                error("No image file name");
        }

        file_name = dir_name / file_name;

        if (auto iter = image_index->find(file_name); iter != image_index->end())
        {
                *index = iter->second;
                return;
        }

        images->push_back(read_image_from_file<N - 1>(file_name));
        *index = images->size() - 1;
        image_index->emplace(file_name, *index);
}

// Варианты данных: "x/x/x ...", "x//x ...", "x// ...", "x/x/ ...", "x/x ...", "x ...".
template <typename T, size_t MaxGroupCount, size_t GroupSize, typename IndexType>
void read_digit_groups(
        const T& line,
        long long begin,
        long long end,
        std::array<std::array<IndexType, GroupSize>, MaxGroupCount>* group_ptr,
        int* group_count)
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

                std::array<IndexType, GroupSize>& indices = (*group_ptr)[group_index];

                // Считывается номер вершины
                if (read_integer(line, end, &i, &indices[0]))
                {
                        if (indices[0] == 0)
                        {
                                error("Zero facet index");
                        }
                }
                else
                {
                        error("Error read facet vertex first number");
                }

                // Считываются текстура и нормаль
                for (int a = 1; a < static_cast<int>(indices.size()); ++a)
                {
                        if (i == end || ascii::is_space(line[i]))
                        {
                                indices[a] = 0;
                                continue;
                        }

                        if (!is_solidus(line[i]))
                        {
                                error("Error read facet vertex number");
                        }

                        ++i;

                        if (i == end || ascii::is_space(line[i]))
                        {
                                indices[a] = 0;
                                continue;
                        }

                        if (read_integer(line, end, &i, &indices[a]))
                        {
                                if (indices[a] == 0)
                                {
                                        error("Zero facet index");
                                }
                        }
                        else
                        {
                                indices[a] = 0;
                        }
                }
        }
}

// 0 означает, что нет индекса.
// Индексы находятся в порядке facet, texture, normal.
template <typename T, size_t MaxGroupCount>
void check_index_consistent(const std::array<std::array<T, 3>, MaxGroupCount>& groups, int group_count)
{
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

template <size_t N, typename T>
void read_facets(
        const T& data,
        long long begin,
        long long end,
        std::array<typename Mesh<N>::Facet, MAX_FACETS_PER_LINE<N>>* facets,
        int* facet_count)
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

        // Обязательная проверка индексов
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

template <size_t N, typename T>
void read_float_texture(const char* str, Vector<N, T>* v)
{
        T tmp;

        int n = read_vector(str, v, &tmp, std::make_integer_sequence<unsigned, N>()).first;

        if (n != N && n != N + 1)
        {
                error(std::string("Error read " + to_string(N) + " or " + to_string(N + 1) + " floating points of ")
                      + type_name<T>() + " type");
        }

        if (n == N + 1 && tmp != 0)
        {
                error(to_string(N + 1) + "-dimensional textures are not supported");
        }
}

template <typename T>
void read_name(const char* object_name, const T& data, long long begin, long long end, std::string* name)
{
        const long long size = end;

        long long i = begin;
        read(data, size, ascii::is_space, &i);
        if (i == size)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        long long i2 = i;
        read(data, size, ascii::is_not_space, &i2);

        *name = std::string(&data[i], i2 - i);

        i = i2;

        read(data, size, ascii::is_space, &i);
        if (i != size)
        {
                error("Error read " + std::string(object_name) + " name");
        }
}

template <typename T>
void read_library_names(
        const T& data,
        long long begin,
        long long end,
        std::vector<std::filesystem::path>* v,
        std::set<std::filesystem::path>* lib_unique_names)
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

// Разделение строки на 2 части " не_пробелы | остальной текст до символа комментария или конца строки"
template <typename T, typename Space, typename Comment>
void split(
        const T& data,
        long long first,
        long long last,
        const Space& space,
        const Comment& comment,
        long long* first_b,
        long long* first_e,
        long long* second_b,
        long long* second_e)
{
        long long i = first;

        while (i < last && space(data[i]))
        {
                ++i;
        }
        if (i == last || comment(data[i]))
        {
                *first_b = i;
                *first_e = i;
                *second_b = i;
                *second_e = i;
                return;
        }

        long long i2 = i + 1;
        while (i2 < last && !space(data[i2]) && !comment(data[i2]))
        {
                ++i2;
        }
        *first_b = i;
        *first_e = i2;

        i = i2;

        if (i == last || comment(data[i]))
        {
                *second_b = i;
                *second_e = i;
                return;
        }

        // первый пробел пропускается
        ++i;

        i2 = i;
        while (i2 < last && !comment(data[i2]))
        {
                ++i2;
        }

        *second_b = i;
        *second_e = i2;
}

template <typename T>
void split_line(
        T* data,
        const std::vector<long long>& line_begin,
        long long line_num,
        const char** first,
        const char** second,
        long long* second_b,
        long long* second_e)
{
        long long line_count = line_begin.size();

        long long last = (line_num + 1 < line_count) ? line_begin[line_num + 1] : data->size();

        // В конце строки находится символ '\n', сместиться на него
        --last;

        long long first_b;
        long long first_e;

        split(*data, line_begin[line_num], last, ascii::is_space, is_number_sign, &first_b, &first_e, second_b,
              second_e);

        *first = &(*data)[first_b];
        (*data)[first_e] = 0; // пробел, символ комментария '#' или символ '\n'

        *second = &(*data)[*second_b];
        (*data)[*second_e] = 0; // символ комментария '#' или символ '\n'
}

template <size_t N>
bool facet_dimension_is_correct(const std::vector<Vector<N, float>>& vertices, const std::array<int, N>& indices)
{
        static_assert(N == 3);

        Vector<3, double> e0 = to_vector<double>(vertices[indices[1]] - vertices[indices[0]]);
        Vector<3, double> e1 = to_vector<double>(vertices[indices[2]] - vertices[indices[0]]);

        // Перебрать все возможные определители 2x2.
        // Здесь достаточно просто сравнить с 0.

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
        v,
        vt,
        vn,
        f,
        usemtl,
        mtllib,
        None,
        NotSupported
};

template <size_t N>
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

template <size_t N>
void check_facet_indices(const Mesh<N>& mesh)
{
        int vertex_count = mesh.vertices.size();
        int texcoord_count = mesh.texcoords.size();
        int normal_count = mesh.normals.size();

        for (const typename Mesh<N>::Facet& facet : mesh.facets)
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
}

template <size_t N>
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

                for (size_t i = 0; i < mesh->facets.size(); ++i)
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

                for (size_t i = 0; i < mesh->facets.size(); ++i)
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

template <size_t N>
void read_obj_stage_one(
        unsigned thread_num,
        unsigned thread_count,
        std::vector<Counters>* counters,
        std::vector<char>* data_ptr,
        std::vector<long long>* line_begin,
        std::vector<ObjLine<N>>* line_prop,
        ProgressRatio* progress)
{
        ASSERT(counters->size() == thread_count);

        std::vector<char>& data = *data_ptr;
        const long long line_count = line_begin->size();
        const double line_count_reciprocal = 1.0 / line_begin->size();

        for (long long line_num = thread_num; line_num < line_count; line_num += thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                ObjLine<N> lp;

                const char* first;
                const char* second;

                split_line(&data, *line_begin, line_num, &first, &second, &lp.second_b, &lp.second_e);

                try
                {
                        if (str_equal(first, OBJ_v))
                        {
                                lp.type = ObjLineType::v;
                                Vector<N, float> v;
                                read_float(&data[lp.second_b], &v);
                                lp.v = v;

                                ++((*counters)[thread_num].vertex);
                        }
                        else if (str_equal(first, OBJ_vt))
                        {
                                lp.type = ObjLineType::vt;
                                Vector<N - 1, float> v;
                                read_float_texture(&data[lp.second_b], &v);
                                for (unsigned i = 0; i < N - 1; ++i)
                                {
                                        lp.v[i] = v[i];
                                }

                                ++((*counters)[thread_num].texcoord);
                        }
                        else if (str_equal(first, OBJ_vn))
                        {
                                lp.type = ObjLineType::vn;
                                Vector<N, float> v;
                                read_float(&data[lp.second_b], &v);
                                lp.v = v.normalized();
                                if (!is_finite(lp.v))
                                {
                                        lp.v = Vector<N, float>(0);
                                }

                                ++((*counters)[thread_num].normal);
                        }
                        else if (str_equal(first, OBJ_f))
                        {
                                lp.type = ObjLineType::f;
                                read_facets<N>(data, lp.second_b, lp.second_e, &lp.facets, &lp.facet_count);

                                ++((*counters)[thread_num].facet);
                        }
                        else if (str_equal(first, OBJ_usemtl))
                        {
                                lp.type = ObjLineType::usemtl;
                        }
                        else if (str_equal(first, OBJ_mtllib))
                        {
                                lp.type = ObjLineType::mtllib;
                        }
                        else if (!*first)
                        {
                                lp.type = ObjLineType::None;
                        }
                        else
                        {
                                lp.type = ObjLineType::NotSupported;
                        }
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(line_num) + ": " + first + " " + second + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line_num) + ": " + first + " " + second + "\n" + "Unknown error");
                }

                (*line_prop)[line_num] = lp;
        }
}

// Индексы в OBJ:
//   начинаются с 1 для абсолютных значений,
//   начинаются с -1 для относительных значений назад.
// Преобразование в абсолютные значения с началом от 0.
template <size_t N>
void correct_indices(typename Mesh<N>::Facet* facet, int vertices_size, int texcoords_size, int normals_size)
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

template <size_t N>
void read_obj_stage_two(
        const Counters& counters,
        std::vector<char>* data_ptr,
        std::vector<ObjLine<N>>* line_prop,
        ProgressRatio* progress,
        std::map<std::string, int>* material_index,
        std::vector<std::filesystem::path>* library_names,
        Mesh<N>* mesh)
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
                case ObjLineType::v:
                        mesh->vertices.push_back(lp.v);
                        break;
                case ObjLineType::vt:
                {
                        mesh->texcoords.resize(mesh->texcoords.size() + 1);
                        Vector<N - 1, float>& new_vector = mesh->texcoords[mesh->texcoords.size() - 1];
                        for (unsigned i = 0; i < N - 1; ++i)
                        {
                                new_vector[i] = lp.v[i];
                        }
                        break;
                }
                case ObjLineType::vn:
                        mesh->normals.push_back(lp.v);
                        break;
                case ObjLineType::f:
                        for (int i = 0; i < lp.facet_count; ++i)
                        {
                                lp.facets[i].material = mtl_index;
                                correct_indices<N>(
                                        &lp.facets[i], mesh->vertices.size(), mesh->texcoords.size(),
                                        mesh->normals.size());
                                mesh->facets.push_back(std::move(lp.facets[i]));
                        }
                        break;
                case ObjLineType::usemtl:
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
                case ObjLineType::mtllib:
                        read_library_names(data, lp.second_b, lp.second_e, library_names, &unique_library_names);
                        break;
                case ObjLineType::None:
                case ObjLineType::NotSupported:
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

template <size_t N>
void read_obj_thread(
        unsigned thread_num,
        unsigned thread_count,
        std::vector<Counters>* counters,
        ThreadBarrier* barrier,
        std::atomic_bool* error_found,
        std::vector<char>* data_ptr,
        std::vector<long long>* line_begin,
        std::vector<ObjLine<N>>* line_prop,
        ProgressRatio* progress,
        std::map<std::string, int>* material_index,
        std::vector<std::filesystem::path>* library_names,
        Mesh<N>* mesh)
{
        // параллельно

        try
        {
                read_obj_stage_one(thread_num, thread_count, counters, data_ptr, line_begin, line_prop, progress);
        }
        catch (...)
        {
                error_found->store(true); // нет исключений
                barrier->wait();
                throw;
        }
        barrier->wait();
        if (*error_found)
        {
                return;
        }

        if (thread_num != 0)
        {
                return;
        }

        //последовательно

        line_begin->clear();
        line_begin->shrink_to_fit();

        read_obj_stage_two(sum_counters(*counters), data_ptr, line_prop, progress, material_index, library_names, mesh);
}

template <size_t N>
void read_lib(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& file_name,
        ProgressRatio* progress,
        std::map<std::string, int>* material_index,
        std::map<std::string, int>* image_index,
        Mesh<N>* mesh)
{
        std::vector<char> data;
        std::vector<long long> line_begin;

        const std::filesystem::path lib_name = dir_name / file_name;

        read_file_lines(lib_name, &data, &line_begin);

        const std::filesystem::path lib_dir = lib_name.parent_path();

        typename Mesh<N>::Material* mtl = nullptr;
        std::string name;

        const long long line_count = line_begin.size();
        const double line_count_reciprocal = 1.0 / line_begin.size();

        for (long long line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                const char* first;
                const char* second;
                long long second_b;
                long long second_e;

                split_line(&data, line_begin, line_num, &first, &second, &second_b, &second_e);

                try
                {
                        if (!*first)
                        {
                        }
                        else if (str_equal(first, MTL_newmtl))
                        {
                                if (material_index->empty())
                                {
                                        // все материалы найдены
                                        break;
                                }

                                read_name("material", data, second_b, second_e, &name);

                                auto iter = material_index->find(name);
                                if (iter != material_index->end())
                                {
                                        mtl = &(mesh->materials[iter->second]);
                                        material_index->erase(name);
                                }
                                else
                                {
                                        // ненужный материал
                                        mtl = nullptr;
                                }
                        }
                        else if (str_equal(first, MTL_Ka))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_float(&data[second_b], &mtl->Ka.data());

                                if (!check_color(mtl->Ka))
                                {
                                        error("Error Ka in material " + mtl->name);
                                }
                        }
                        else if (str_equal(first, MTL_Kd))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_float(&data[second_b], &mtl->Kd.data());

                                if (!check_color(mtl->Kd))
                                {
                                        error("Error Kd in material " + mtl->name);
                                }
                        }
                        else if (str_equal(first, MTL_Ks))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_float(&data[second_b], &mtl->Ks.data());

                                if (!check_color(mtl->Ks))
                                {
                                        error("Error Ks in material " + mtl->name);
                                }
                        }
                        else if (str_equal(first, MTL_Ns))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_float(&data[second_b], &mtl->Ns);

                                if (!check_range(mtl->Ns, 0, 1000))
                                {
                                        error("Error Ns in material " + mtl->name);
                                }
                        }
                        else if (str_equal(first, MTL_map_Ka))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_name("file", data, second_b, second_e, &name);
                                load_image<N>(lib_dir, name, image_index, &mesh->images, &mtl->map_Ka);
                        }
                        else if (str_equal(first, MTL_map_Kd))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_name("file", data, second_b, second_e, &name);
                                load_image<N>(lib_dir, name, image_index, &mesh->images, &mtl->map_Kd);
                        }
                        else if (str_equal(first, MTL_map_Ks))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_name("file", data, second_b, second_e, &name);
                                load_image<N>(lib_dir, name, image_index, &mesh->images, &mtl->map_Ks);
                        }
                }
                catch (const std::exception& e)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(line_num)
                              + ": " + first + " " + second + "\n" + e.what());
                }
                catch (...)
                {
                        error("Library: " + generic_utf8_filename(lib_name) + "\n" + "Line " + to_string(line_num)
                              + ": " + first + " " + second + "\n" + "Unknown error");
                }
        }
}

template <size_t N>
void read_libs(
        const std::filesystem::path& dir_name,
        ProgressRatio* progress,
        std::map<std::string, int>* material_index,
        const std::vector<std::filesystem::path>& library_names,
        Mesh<N>* mesh)
{
        std::map<std::string, int> image_index;

        for (size_t i = 0; (i < library_names.size()) && !material_index->empty(); ++i)
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

template <size_t N>
void read_obj(
        const std::filesystem::path& file_name,
        ProgressRatio* progress,
        std::map<std::string, int>* material_index,
        std::vector<std::filesystem::path>* library_names,
        Mesh<N>* mesh)
{
        const int thread_count = hardware_concurrency();

        std::vector<char> data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &data, &line_begin);

        std::vector<ObjLine<N>> line_prop(line_begin.size());
        ThreadBarrier barrier(thread_count);
        std::atomic_bool error_found{false};
        std::vector<Counters> counters(thread_count);

        ThreadsWithCatch threads(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
                threads.add(
                        [&, i]()
                        {
                                read_obj_thread(
                                        i, thread_count, &counters, &barrier, &error_found, &data, &line_begin,
                                        &line_prop, progress, material_index, library_names, mesh);
                        });
        }
        threads.join();
}

template <size_t N>
std::unique_ptr<Mesh<N>> read_obj_and_mtl(const std::filesystem::path& file_name, ProgressRatio* progress)
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

template <size_t N>
std::unique_ptr<Mesh<N>> load_from_obj_file(const std::filesystem::path& file_name, ProgressRatio* progress)
{
        TimePoint start_time = time();

        std::unique_ptr<Mesh<N>> mesh = read_obj_and_mtl<N>(file_name, progress);

        LOG(obj_type_name(N) + " loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_obj_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<4>> load_from_obj_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<5>> load_from_obj_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<6>> load_from_obj_file(const std::filesystem::path& file_name, ProgressRatio* progress);
}
