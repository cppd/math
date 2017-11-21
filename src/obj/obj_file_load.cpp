/*
Copyright (C) 2017 Topological Manifold

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

#include "obj_file_load.h"

#include "obj_alg.h"

#include "com/error.h"
#include "com/file.h"
#include "com/file_sys.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/str.h"
#include "com/thread.h"
#include "com/time.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <thread>

using atomic_counter = AtomicCounter<int>;
constexpr bool ATOMIC_COUNTER_LOCK_FREE = atomic_counter::is_always_lock_free;

constexpr unsigned MAX_FACES_PER_LINE = 5;

namespace
{
template <typename T>
void read(const std::string& line, size_t size, const T& op, size_t* i)
{
        while (*i < size && op(line[*i]))
        {
                ++(*i);
        }
}

void read_integer(const std::string& line, size_t size, size_t* i)
{
        if (*i < size && line[*i] == '-')
        {
                size_t pos = *i + 1;
                read(line, size, [](char c) { return std::isdigit(c); }, &pos);
                if (pos > (*i + 1))
                {
                        *i = pos;
                }
        }
        else
        {
                read(line, size, [](char c) { return std::isdigit(c); }, i);
        }
}

template <typename T>
std::string get_string_list(const std::map<std::string, T>& m)
{
        std::string names;
        for (const auto& s : m)
        {
                if (names.size() > 0)
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

bool check_range(float v, float min, float max)
{
        return v >= min && v <= max;
}
bool check_range(const vec3f& v, float min, float max)
{
        return v[0] >= min && v[0] <= max && v[1] >= min && v[1] <= max && v[2] >= min && v[2] <= max;
}

void read_text_file(const std::string& file, std::string* s)
{
#if 0

        CFile f(file, "rb");

        std::fseek(f, 0, SEEK_END);
        size_t length = std::ftell(f);

        if (length == 0)
        {
                s->clear();
                return;
        }

        std::fseek(f, -1, SEEK_END);
        char c;
        std::fscanf(f, "%c", &c);
        if (c == '\n')
        {
                s->resize(length);
        }
        else
        {
                s->resize(length + 1);
                (*s)[s->size() - 1] = '\n';
        }

        std::rewind(f);
        std::fread(&(*s)[0], length, 1, f);

#else

        std::ifstream f(file);
        if (!f)
        {
                error("error open file " + file);
        }

        f.seekg(0, f.end);
        size_t length = f.tellg();
        if (length == 0)
        {
                s->clear();
                return;
        }

        f.seekg(-1, f.end);
        if (f.get() == '\n')
        {
                s->resize(length);
        }
        else
        {
                s->resize(length + 1);
                (*s)[s->size() - 1] = '\n';
        }

        f.seekg(0, f.beg);
        f.read(&(*s)[0], length);

#endif
}

void get_file_lines(const std::string& file, std::string* s, std::vector<size_t>* line_begin)
{
        read_text_file(file, s);

        size_t size = s->size();

        size_t cnt = 0;
        for (size_t i = 0; i < size; ++i)
        {
                if ((*s)[i] == '\n')
                {
                        ++cnt;
                }
        }

        line_begin->clear();
        line_begin->reserve(cnt);

        size_t b = 0;
        for (size_t i = 0; i < size; ++i)
        {
                if ((*s)[i] == '\n')
                {
                        line_begin->push_back(b);
                        b = i + 1;
                }
        }
}

void load_image(const std::string& dir_name, std::string&& name, std::map<std::string, int>* image_index,
                std::vector<sf::Image>* images, int* index)
{
        name = trim(name);
        if (name.size() == 0)
        {
                error("no map file name");
        }

#if defined(__linux__)
        // путь к файлу может быть указан в формате Windows, поэтому надо заменить разделители
        std::replace(name.begin(), name.end(), '\\', '/');
#endif

        name = dir_name + "/" + name;

        auto iter = image_index->find(name);
        if (iter != image_index->end())
        {
                *index = iter->second;
                return;
        };

        images->push_back(sf::Image());
        *index = images->size() - 1;
        if (!(*images)[*index].loadFromFile(name))
        {
                error("error open file " + name);
        }

        image_index->emplace(name, *index);
}

// Между begin и end находится уже проверенное целое число в формате DDDDD или -DDDDD
int string_to_integer(const std::string& s, long long begin, long long end)
{
        bool neg = false;
        if (s[begin] == '-')
        {
                neg = true;
                ++begin;
        }

        if (end - begin > 9)
        {
                error("Error convert to int (too big): " + s);
        }

        long long sum = 0, mul = 1;
        for (long long i = end - 1; i >= begin; --i)
        {
                long long v = s[i] - '0';
                sum += v * mul;
                mul *= 10;
        }

        return neg ? -sum : sum;
}

// 0 означает, что нет индекса.
// Индексы находятся в порядке face, texture, normal.
template <typename T>
void check_indices(const T& v, unsigned group_count, const char* line_begin)
{
        int end = group_count * 3;

        ASSERT(end <= static_cast<int>(v.size()));

        for (int idx = 0; idx < end; idx += 3)
        {
                if (v[idx] == 0)
                {
                        error("Error read face from line:\n\"" + trim(line_begin) + "\"");
                }
        }

        for (int idx = 1; idx < end - 3; idx += 3)
        {
                if ((v[idx] == 0) != (v[idx + 3] == 0))
                {
                        error("Inconsistent face texture indices in the line:\n\"" + trim(line_begin) + "\"");
                }
        }

        for (int idx = 2; idx < end - 3; idx += 3)
        {
                if ((v[idx] == 0) != (v[idx + 3] == 0))
                {
                        error("Inconsistent face normal indices in the line:\n\"" + trim(line_begin) + "\"");
                }
        }
}

template <size_t N>
void read_vertex_groups(const std::string& line, const char* line_begin, size_t begin, size_t end, std::array<size_t, N>* begins,
                        std::array<size_t, N>* ends, unsigned* cnt)
{
        size_t i = begin;
        *cnt = 0;
        for (unsigned z = 0; z < N + 1; ++z)
        {
                read(line, end, [](char c) { return std::isspace(c); }, &i);
                size_t i2 = i;
                read(line, end, [](char c) { return !std::isspace(c); }, &i2);
                if (i2 != i)
                {
                        if (z < N)
                        {
                                (*begins)[z] = i;
                                (*ends)[z] = i2;
                                i = i2;
                        }
                        else
                        {
                                error("Too many vertex groups (max=" + to_string(N) + ") in line:\n\"" + trim(line_begin) + "\"");
                        }
                }
                else
                {
                        *cnt = z;
                        return;
                }
        }
}

void read_v_vt_vn(const std::string& line, const char* line_begin, size_t begin, size_t end, int* v)
{
        size_t i = begin;
        for (int a = 0; a < 3; ++a)
        {
                if (i == end)
                {
                        if (a > 0) // a > 0 — считывается текстура или нормаль
                        {
                                v[a] = 0;
                                continue;
                        }
                        error("Error read face from line:\n\"" + trim(line_begin) + "\"");
                }

                if (a > 0)
                {
                        if (line[i] != '/')
                        {
                                error("Error read face from line:\n\"" + trim(line_begin) + "\"");
                        }
                        ++i;
                }

                size_t i2 = i;

                read_integer(line, end, &i2);
                if (i2 != i)
                {
                        v[a] = string_to_integer(line, i, i2);
                        if (v[a] == 0)
                        {
                                error("Zero face index:\n\"" + trim(line_begin) + "\"");
                        }
                }
                else
                {
                        if (a == 0) // a == 0 — считывается вершина
                        {
                                error("Error read face from line:\n\"" + trim(line_begin) + "\"");
                        }

                        v[a] = 0;
                }

                i = i2;
        }

        if (i != end)
        {
                error("Error read face from line:\n\"" + trim(line_begin) + "\"");
        }
}

// Разделение строки на 9 чисел
// " число/возможно_число/возможно_число число/возможно_число/возможно_число число/возможно_число/возможно_число ".
// Примеры: " 1/2/3 4/5/6 7/8/9", "1//2 3//4 5//6", " 1// 2// 3// ".
void read_faces(const std::string& line, size_t begin, size_t end, std::array<IObj::face3, MAX_FACES_PER_LINE>* faces,
                unsigned* face_count)

{
        constexpr unsigned MAX_GROUP_COUNT = MAX_FACES_PER_LINE + 2;

        std::array<int, MAX_GROUP_COUNT * 3> v;
        std::array<size_t, MAX_GROUP_COUNT> begins, ends;

        unsigned group_count;

        read_vertex_groups(line, &line[begin], begin, end, &begins, &ends, &group_count);

        if (group_count < 3)
        {
                error("Error read at least 3 vertices from line:\n\"" + trim(&line[begin]) + "\"");
        }

        for (unsigned z = 0; z < group_count; ++z)
        {
                read_v_vt_vn(line, &line[begin], begins[z], ends[z], &v[z * 3]);
        }

        // Обязательная проверка индексов
        check_indices(v, group_count, &line[begin]);

        *face_count = group_count - 2;

        for (unsigned i = 0, base = 0; i < *face_count; ++i, base += 3)
        {
                (*faces)[i].has_vt = !(v[1] == 0);
                (*faces)[i].has_vn = !(v[2] == 0);

                (*faces)[i].vertices[0].v = v[0];
                (*faces)[i].vertices[0].vt = v[1];
                (*faces)[i].vertices[0].vn = v[2];

                (*faces)[i].vertices[1].v = v[base + 3];
                (*faces)[i].vertices[1].vt = v[base + 4];
                (*faces)[i].vertices[1].vn = v[base + 5];
                (*faces)[i].vertices[2].v = v[base + 6];
                (*faces)[i].vertices[2].vt = v[base + 7];
                (*faces)[i].vertices[2].vn = v[base + 8];
        }
}

void read_float(const std::string& line, size_t b, size_t, vec3f* v)
{
        if (3 != std::sscanf(&line[b], "%f %f %f", &(*v)[0], &(*v)[1], &(*v)[2]))
        {
                std::string l = &line[b];
                error("error read 3 floating points from line:\n\"" + trim(l) + "\"");
        }
}

void read_float_texture(const std::string& line, size_t b, size_t, vec2f* v)
{
        float tmp;

        int n = std::sscanf(&line[b], "%f %f %f", &(*v)[0], &(*v)[1], &tmp);
        if (n != 2 && n != 3)
        {
                std::string l = &line[b];
                error("error read 2 or 3 floating points from line:\n\"" + trim(l) + "\"");
        }
        if (n == 3 && tmp != 0.0f)
        {
                std::string l = &line[b];
                error("3D textures not supported:\n\"" + trim(l) + "\"");
        }
}

void read_float(const std::string& line, size_t b, size_t, float* v)
{
        if (1 != std::sscanf(&line[b], "%f", v))
        {
                std::string l = &line[b];
                error("error read 1 floating point from line:\n\"" + trim(l) + "\"");
        }
}

void read_mtl_name(const std::string& line, size_t b, size_t e, std::string* res)
{
        const size_t size = e;

        size_t i = b;
        read(line, size, [](char c) { return std::isspace(c); }, &i);
        if (i == size)
        {
                std::string l = &line[b];
                error("Error read material name from line:\n\"" + trim(l) + "\"");
        }

        size_t i2 = i;
        read(line, size, [](char c) { return !std::isspace(c); }, &i2);
        *res = line.substr(i, i2 - i);
        i = i2;

        read(line, size, [](char c) { return std::isspace(c); }, &i);
        if (i != size)
        {
                std::string l = &line[b];
                error("Error read material name from line:\n\"" + trim(l) + "\"");
        }
}

void read_library_names(const std::string& line, size_t b, size_t e, std::vector<std::string>* v,
                        std::set<std::string>* lib_unique_names)
{
        const size_t size = e;
        bool found = false;
        size_t i = b;

        while (true)
        {
                read(line, size, [](char c) { return std::isspace(c); }, &i);
                if (i == size)
                {
                        if (!found)
                        {
                                std::string l = &line[b];
                                error("Library name not found in line:\n\"" + trim(l) + "\"");
                        }
                        return;
                }

                size_t i2 = i;
                read(line, size, [](char c) { return !std::isspace(c); }, &i2);
                std::string name{line.substr(i, i2 - i)};
                i = i2;
                found = true;

                if (lib_unique_names->find(name) == lib_unique_names->end())
                {
                        v->push_back(name);
                        lib_unique_names->insert(std::move(name));
                }
        }
}

bool compare(const std::string& str, size_t b, size_t e, const std::string& tag)
{
        if ((e - b) != tag.size())
        {
                return false;
        }
        return !strcmp(&str[b], tag.c_str());
}

// Разделение строки на 2 части " не_пробелы | остальной текст до символа комментария или конца строки"
template <typename T, typename C>
void split(const std::string& line, size_t begin, size_t end, const T& space, const C& comment, size_t* first_b, size_t* first_e,
           size_t* second_b, size_t* second_e)
{
        const size_t size = end;

        size_t i = begin;
        while (i < size && space(line[i]) && !comment(line[i]))
        {
                ++i;
        }
        if (i == size || comment(line[i]))
        {
                *first_b = i;
                *first_e = i;
                *second_b = i;
                *second_e = i;
                return;
        }

        size_t i2 = i + 1;
        while (i2 < size && !space(line[i2]) && !comment(line[i2]))
        {
                ++i2;
        }
        *first_b = i;
        *first_e = i2;

        i = i2;

        if (i == size || comment(line[i]))
        {
                *second_b = i;
                *second_e = i;
                return;
        }

        // первый пробел пропускается
        ++i;

        i2 = i;
        while (i2 < size && !comment(line[i2]))
        {
                ++i2;
        }

        *second_b = i;
        *second_e = i2;
}

template <typename T>
void split_line(std::string* file_str, const std::vector<T>& line_begin, T line_num, T* first_b, T* first_e, T* second_b,
                T* second_e)
{
        T l_b = line_begin[line_num];
        T l_e = (line_num + 1 < line_begin.size()) ? line_begin[line_num + 1] : file_str->size();

        // В конце строки находится символ '\n', сместиться на него.
        --l_e;

        split(*file_str, l_b, l_e, [](char c) { return std::isspace(c); }, [](char c) { return c == '#'; }, first_b, first_e,
              second_b, second_e);
        (*file_str)[*first_e] = 0; // пробел, символ комментария '#' или символ '\n'
        (*file_str)[*second_e] = 0; // символ комментария '#' или символ '\n'
}

bool face_is_one_dimensional(const vec3f& v0, const vec3f& v1, const vec3f& v2)
{
        Vector<3, double> e0 = to_vector<double>(v1 - v0);
        Vector<3, double> e1 = to_vector<double>(v2 - v0);

        // Перебрать все возможные определители 2x2.
        // Здесь достаточно просто сравнить с 0.

        if (e0[1] * e1[2] - e0[2] * e1[1] != 0)
        {
                return false;
        }

        if (e0[0] * e1[2] - e0[2] * e1[0] != 0)
        {
                return false;
        }

        if (e0[0] * e1[1] - e0[1] * e1[0] != 0)
        {
                return false;
        }

        return true;
}

class FileObj final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<face3> m_faces;
        std::vector<int> m_points;
        std::vector<std::array<int, 2>> m_lines;
        std::vector<material> m_materials;
        std::vector<sf::Image> m_images;
        vec3f m_center;
        float m_length;

        const std::string tag_v{"v"}, tag_vt{"vt"}, tag_vn{"vn"}, tag_f{"f"}, tag_usemtl{"usemtl"}, tag_mtllib{"mtllib"};
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

        const std::string tag_newmtl{"newmtl"}, tag_Ka{"Ka"}, tag_Kd{"Kd"}, tag_Ks{"Ks"}, tag_Ns{"Ns"}, tag_map_Ka{"map_Ka"},
                tag_map_Kd{"map_Kd"}, tag_map_Ks{"map_Ks"};
        enum class MtlLineType
        {
                NEWMTL,
                KA,
                KD,
                KS,
                NS,
                MAP_KA,
                MAP_KD,
                MAP_KS,
                NONE,
                NOT_SUPPORTED
        };

        struct ObjLine
        {
                ObjLineType type;
                size_t second_b, second_e;
                std::array<face3, MAX_FACES_PER_LINE> faces;
                unsigned face_count;
                vec3f v;
        };

        struct MtlLine
        {
                MtlLineType type;
                size_t second_b, second_e;
                vec3f v;
        };

        struct ThreadData
        {
                size_t thread_num;
                size_t thread_count;
                ThreadBarrier* barrier;
                atomic_counter* cnt_v;
                atomic_counter* cnt_vt;
                atomic_counter* cnt_vn;
                atomic_counter* cnt_f;
                std::atomic_bool* error_found;
        };

        void check_face_indices() const;

        bool remove_one_dimensional_faces();

        void read_obj_one(const ThreadData* thread_data, std::string* file_ptr, std::vector<size_t>* line_begin,
                          std::vector<ObjLine>* line_prop, ProgressRatio* progress) const;
        void read_obj_two(const ThreadData* thread_data, std::string* file_ptr, std::vector<ObjLine>* line_prop,
                          ProgressRatio* progress, std::map<std::string, int>* material_index,
                          std::vector<std::string>* library_names);
        void read_obj_th(const ThreadData* thread_data, std::string* file_str, std::vector<size_t>* line_begin,
                         std::vector<ObjLine>* line_prop, ProgressRatio* progress, std::map<std::string, int>* material_index,
                         std::vector<std::string>* library_names);
        void read_obj(const std::string& file_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                      std::vector<std::string>* library_names);

        void read_lib(const std::string& dir_name, const std::string& file_name, ProgressRatio* progress,
                      std::map<std::string, int>* material_index, std::map<std::string, int>* image_index);
        void read_libs(const std::string& dir_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                       const std::vector<std::string>& library_names);

        void read_obj_and_mtl(const std::string& file_name, ProgressRatio* progress);

        const std::vector<vec3f>& get_vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& get_texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& get_normals() const override
        {
                return m_normals;
        }
        const std::vector<face3>& get_faces() const override
        {
                return m_faces;
        }
        const std::vector<int>& get_points() const override
        {
                return m_points;
        }
        const std::vector<std::array<int, 2>>& get_lines() const override
        {
                return m_lines;
        }
        const std::vector<material>& get_materials() const override
        {
                return m_materials;
        }
        const std::vector<sf::Image>& get_images() const override
        {
                return m_images;
        }
        vec3f get_center() const override
        {
                return m_center;
        }
        float get_length() const override
        {
                return m_length;
        }

public:
        FileObj(const std::string& file_name, ProgressRatio* progress);
};

void FileObj::check_face_indices() const
{
        int vertex_count = m_vertices.size();
        int texcoord_count = m_texcoords.size();
        int normal_count = m_normals.size();

        for (const face3& face : m_faces)
        {
                for (int i = 0; i < 3; ++i)
                {
                        if (face.vertices[i].v < 0 || face.vertices[i].v >= vertex_count)
                        {
                                error("vertex index " + std::to_string(face.vertices[i].v) +
                                      " is zero or out of the vertex count " + std::to_string(vertex_count));
                        }
                        if (face.has_vt && (face.vertices[i].vt < 0 || face.vertices[i].vt >= texcoord_count))
                        {
                                error("texture coord index " + std::to_string(face.vertices[i].vt) +
                                      " is zero or out of the texture coord count " + std::to_string(texcoord_count));
                        }
                        if (face.has_vn && (face.vertices[i].vn < 0 || face.vertices[i].vn >= normal_count))
                        {
                                error("normal index " + std::to_string(face.vertices[i].vn) +
                                      " is zero or out of the normal count " + std::to_string(normal_count));
                        }
                }
        }
}

bool FileObj::remove_one_dimensional_faces()
{
        std::vector<bool> one_d_faces(m_faces.size(), false);

        int one_d_face_count = 0;

        for (unsigned i = 0; i < m_faces.size(); ++i)
        {
                vec3f v0 = m_vertices[m_faces[i].vertices[0].v];
                vec3f v1 = m_vertices[m_faces[i].vertices[1].v];
                vec3f v2 = m_vertices[m_faces[i].vertices[2].v];

                if (face_is_one_dimensional(v0, v1, v2))
                {
                        one_d_faces[i] = true;
                        ++one_d_face_count;
                }
        }

        if (one_d_face_count == 0)
        {
                return false;
        }

        std::vector<face3> faces;
        faces.reserve(m_faces.size() - one_d_face_count);

        for (unsigned i = 0; i < m_faces.size(); ++i)
        {
                if (!one_d_faces[i])
                {
                        faces.push_back(m_faces[i]);
                }
        }

        m_faces = std::move(faces);

        return true;
}

void FileObj::read_obj_one(const ThreadData* thread_data, std::string* file_ptr, std::vector<size_t>* line_begin,
                           std::vector<ObjLine>* line_prop, ProgressRatio* progress) const
{
        std::string& file_str = *file_ptr;
        const size_t line_count = line_begin->size();
        const double line_count_d = line_begin->size();

        for (size_t line_num = thread_data->thread_num; line_num < line_count; line_num += thread_data->thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num / line_count_d);
                }

                ObjLine lp;
                size_t first_b, first_e;

                split_line(&file_str, *line_begin, line_num, &first_b, &first_e, &lp.second_b, &lp.second_e);

                if (first_b == first_e)
                {
                        lp.type = ObjLineType::NONE;
                }
                else if (compare(file_str, first_b, first_e, tag_v))
                {
                        lp.type = ObjLineType::V;
                        vec3f v;
                        read_float(file_str, lp.second_b, lp.second_e, &v);
                        lp.v = v;
                        if (ATOMIC_COUNTER_LOCK_FREE)
                        {
                                ++(*thread_data->cnt_v);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_vt))
                {
                        lp.type = ObjLineType::VT;
                        vec2f v;
                        read_float_texture(file_str, lp.second_b, lp.second_e, &v);
                        lp.v[0] = v[0];
                        lp.v[1] = v[1];
                        if (ATOMIC_COUNTER_LOCK_FREE)
                        {
                                ++(*thread_data->cnt_vt);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_vn))
                {
                        lp.type = ObjLineType::VN;
                        vec3f v;
                        read_float(file_str, lp.second_b, lp.second_e, &v);
                        lp.v = normalize(v);
                        if (ATOMIC_COUNTER_LOCK_FREE)
                        {
                                ++(*thread_data->cnt_vn);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_f))
                {
                        lp.type = ObjLineType::F;
                        read_faces(file_str, lp.second_b, lp.second_e, &lp.faces, &lp.face_count);
                        if (ATOMIC_COUNTER_LOCK_FREE)
                        {
                                ++(*thread_data->cnt_f);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_usemtl))
                {
                        lp.type = ObjLineType::USEMTL;
                }
                else if (compare(file_str, first_b, first_e, tag_mtllib))
                {
                        lp.type = ObjLineType::MTLLIB;
                }
                else
                {
                        lp.type = ObjLineType::NOT_SUPPORTED;
                }

                (*line_prop)[line_num] = lp;
        }
}

// Индексы в OBJ:
//   начинаются с 1 для абсолютных значений,
//   начинаются с -1 для относительных значений назад.
// Преобразование в абсолютные значения с началом от 0.
void correct_indices(IObj::face3* face, int vertices_size, int texcoords_size, int normals_size)
{
        for (int i = 0; i < 3; ++i)
        {
                int& v = face->vertices[i].v;
                int& vt = face->vertices[i].vt;
                int& vn = face->vertices[i].vn;

                ASSERT(v != 0);

                v = v > 0 ? v - 1 : vertices_size + v;
                vt = vt > 0 ? vt - 1 : (vt < 0 ? texcoords_size + vt : -1);
                vn = vn > 0 ? vn - 1 : (vn < 0 ? normals_size + vn : -1);
        }
}

void FileObj::read_obj_two(const ThreadData* thread_data, std::string* file_ptr, std::vector<ObjLine>* line_prop,
                           ProgressRatio* progress, std::map<std::string, int>* material_index,
                           std::vector<std::string>* library_names)
{
        if (ATOMIC_COUNTER_LOCK_FREE)
        {
                m_vertices.reserve(*thread_data->cnt_v);
                m_texcoords.reserve(*thread_data->cnt_vt);
                m_normals.reserve(*thread_data->cnt_vn);
                m_faces.reserve(*thread_data->cnt_f);
        }

        std::string& file_str = *file_ptr;
        const size_t line_count = line_prop->size();
        const double line_count_d = line_prop->size();

        int mtl_index = -1;
        std::string mtl_name;
        std::set<std::string> unique_library_names;

        for (size_t line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num / line_count_d);
                }

                ObjLine& lp = (*line_prop)[line_num];

                switch (lp.type)
                {
                case ObjLineType::V:
                        m_vertices.push_back(lp.v);
                        break;
                case ObjLineType::VT:
                        m_texcoords.emplace_back(lp.v[0], lp.v[1]);
                        break;
                case ObjLineType::VN:
                        m_normals.push_back(lp.v);
                        break;
                case ObjLineType::F:
                        for (unsigned i = 0; i < lp.face_count; ++i)
                        {
                                lp.faces[i].material = mtl_index;
                                correct_indices(&lp.faces[i], m_vertices.size(), m_texcoords.size(), m_normals.size());
                                m_faces.push_back(std::move(lp.faces[i]));
                        }
                        break;
                case ObjLineType::USEMTL:
                {
                        read_mtl_name(file_str, lp.second_b, lp.second_e, &mtl_name);
                        auto iter = material_index->find(mtl_name);
                        if (iter != material_index->end())
                        {
                                mtl_index = iter->second;
                        }
                        else
                        {
                                IObj::material mtl;
                                mtl.name = mtl_name;
                                m_materials.push_back(std::move(mtl));
                                material_index->emplace(std::move(mtl_name), m_materials.size() - 1);
                                mtl_index = m_materials.size() - 1;
                        }
                        break;
                }
                case ObjLineType::MTLLIB:
                        read_library_names(file_str, lp.second_b, lp.second_e, library_names, &unique_library_names);
                        break;
                case ObjLineType::NONE:
                        break;
                case ObjLineType::NOT_SUPPORTED:
                        break;
                }
        }

        if (!ATOMIC_COUNTER_LOCK_FREE)
        {
                m_vertices.shrink_to_fit();
                m_texcoords.shrink_to_fit();
                m_normals.shrink_to_fit();
                m_faces.shrink_to_fit();
        }
}

void FileObj::read_obj_th(const ThreadData* thread_data, std::string* file_ptr, std::vector<size_t>* line_begin,
                          std::vector<ObjLine>* line_prop, ProgressRatio* progress, std::map<std::string, int>* material_index,
                          std::vector<std::string>* library_names)
{
        // параллельно

        try
        {
                read_obj_one(thread_data, file_ptr, line_begin, line_prop, progress);
        }
        catch (...)
        {
                thread_data->error_found->store(true); // нет исключений
                thread_data->barrier->wait();
                throw;
        }
        thread_data->barrier->wait();
        if (*thread_data->error_found)
        {
                return;
        }

        if (thread_data->thread_num != 0)
        {
                return;
        }

        //последовательно

        line_begin->clear();
        line_begin->shrink_to_fit();

        read_obj_two(thread_data, file_ptr, line_prop, progress, material_index, library_names);
}

void FileObj::read_lib(const std::string& dir_name, const std::string& file_name, ProgressRatio* progress,
                       std::map<std::string, int>* material_index, std::map<std::string, int>* image_index)
{
        std::string file_str;
        std::vector<size_t> line_begin;

        const std::string lib_name = dir_name + "/" + file_name;

        get_file_lines(lib_name, &file_str, &line_begin);

        const std::string lib_dir = get_dir_name(lib_name);

        FileObj::material* mtl = nullptr;
        std::string mtl_name;

        const size_t line_count = line_begin.size();
        const double line_count_d = line_begin.size();

        for (size_t line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num / line_count_d);
                }

                size_t first_b, first_e, second_b, second_e;

                split_line(&file_str, line_begin, line_num, &first_b, &first_e, &second_b, &second_e);

                if (first_b == first_e)
                {
                        continue;
                }
                else if (compare(file_str, first_b, first_e, tag_newmtl))
                {
                        if (material_index->size() == 0)
                        {
                                // все материалы найдены
                                break;
                        }

                        read_mtl_name(file_str, second_b, second_e, &mtl_name);

                        auto iter = material_index->find(mtl_name);
                        if (iter != material_index->end())
                        {
                                mtl = &(m_materials[iter->second]);
                                material_index->erase(mtl_name);
                        }
                        else
                        {
                                // ненужный материал
                                mtl = nullptr;
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_Ka))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        read_float(file_str, second_b, second_e, &mtl->Ka);

                        if (!check_range(mtl->Ka, 0, 1))
                        {
                                error("Error Ka in material " + mtl->name);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_Kd))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        read_float(file_str, second_b, second_e, &mtl->Kd);

                        if (!check_range(mtl->Kd, 0, 1))
                        {
                                error("Error Kd in material " + mtl->name);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_Ks))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        read_float(file_str, second_b, second_e, &mtl->Ks);

                        if (!check_range(mtl->Ks, 0, 1))
                        {
                                error("Error Ks in material " + mtl->name);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_Ns))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        read_float(file_str, second_b, second_e, &mtl->Ns);

                        if (!check_range(mtl->Ns, 0, 1000))
                        {
                                error("Error Ns in material " + mtl->name);
                        }
                }
                else if (compare(file_str, first_b, first_e, tag_map_Ka))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        std::string s = file_str.substr(second_b, second_e - second_b);
                        load_image(lib_dir, std::move(s), image_index, &m_images, &mtl->map_Ka);
                }
                else if (compare(file_str, first_b, first_e, tag_map_Kd))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        std::string s = file_str.substr(second_b, second_e - second_b);
                        load_image(lib_dir, std::move(s), image_index, &m_images, &mtl->map_Kd);
                }
                else if (compare(file_str, first_b, first_e, tag_map_Ks))
                {
                        if (!mtl)
                        {
                                continue;
                        }
                        std::string s = file_str.substr(second_b, second_e - second_b);
                        load_image(lib_dir, std::move(s), image_index, &m_images, &mtl->map_Ks);
                }
        }
}

void FileObj::read_libs(const std::string& dir_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                        const std::vector<std::string>& library_names)
{
        std::map<std::string, int> image_index;

        for (size_t i = 0; (i < library_names.size()) && (material_index->size() > 0); ++i)
        {
                read_lib(dir_name, library_names[i], progress, material_index, &image_index);
        }

        if (material_index->size() != 0)
        {
                error("Materials not found in libraries: " + get_string_list(*material_index));
        }

        m_materials.shrink_to_fit();
        m_images.shrink_to_fit();
}

void FileObj::read_obj(const std::string& file_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                       std::vector<std::string>* library_names)
{
        int hardware_concurrency = get_hardware_concurrency();

        std::string file_str;
        std::vector<size_t> line_begin;
        get_file_lines(file_name, &file_str, &line_begin);
        std::vector<ObjLine> line_prop(line_begin.size());

        std::vector<std::thread> threads(hardware_concurrency);
        std::vector<ThreadData> thread_data(threads.size());
        std::vector<std::string> thread_messages(threads.size());
        ThreadBarrier barrier(threads.size());
        atomic_counter cnt_v(0), cnt_vt(0), cnt_vn(0), cnt_f(0);
        std::atomic_bool error_found{false};

        for (size_t i = 0; i < thread_data.size(); ++i)
        {
                ThreadData t;
                t.thread_num = i;
                t.thread_count = threads.size();
                t.barrier = &barrier;
                t.cnt_v = &cnt_v;
                t.cnt_vt = &cnt_vt;
                t.cnt_vn = &cnt_vn;
                t.cnt_f = &cnt_f;
                t.error_found = &error_found;

                thread_data[i] = t;
        }

        for (size_t i = 0; i < threads.size(); ++i)
        {
                launch_class_thread(&threads[i], &thread_messages[i], &FileObj::read_obj_th, this, &thread_data[i], &file_str,
                                    &line_begin, &line_prop, progress, material_index, library_names);
        }

        join_threads(&threads, &thread_messages);
}

void FileObj::read_obj_and_mtl(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        std::map<std::string, int> material_index;
        std::vector<std::string> library_names;

        read_obj(file_name, progress, &material_index, &library_names);

        if (m_faces.size() == 0)
        {
                error("No faces found in OBJ file");
        }

        check_face_indices();

        find_center_and_length(m_vertices, m_faces, &m_center, &m_length);

        if (remove_one_dimensional_faces())
        {
                if (m_faces.size() == 0)
                {
                        error("No 2D faces found in OBJ file");
                }
                find_center_and_length(m_vertices, m_faces, &m_center, &m_length);
        }

        read_libs(get_dir_name(file_name), progress, &material_index, library_names);
}

FileObj::FileObj(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = get_time_seconds();

        read_obj_and_mtl(file_name, progress);

        LOG("OBJ loaded, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}

// Чтение вершин из текстового файла. Одна вершина на строку. Три координаты через пробел.
// x y z
// x y z
class FileTxt final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<face3> m_faces;
        std::vector<int> m_points;
        std::vector<std::array<int, 2>> m_lines;
        std::vector<material> m_materials;
        std::vector<sf::Image> m_images;
        vec3f m_center;
        float m_length;

        struct ThreadData
        {
                size_t thread_num;
                size_t thread_count;
        };

        void read_points_th(const ThreadData* thread_data, std::string* file_ptr, std::vector<size_t>* line_begin,
                            std::vector<vec3f>* lines, ProgressRatio* progress) const;
        void read_points(const std::string& file_name, ProgressRatio* progress);
        void read_text(const std::string& file_name, ProgressRatio* progress);

        const std::vector<vec3f>& get_vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& get_texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& get_normals() const override
        {
                return m_normals;
        }
        const std::vector<face3>& get_faces() const override
        {
                return m_faces;
        }
        const std::vector<int>& get_points() const override
        {
                return m_points;
        }
        const std::vector<std::array<int, 2>>& get_lines() const override
        {
                return m_lines;
        }
        const std::vector<material>& get_materials() const override
        {
                return m_materials;
        }
        const std::vector<sf::Image>& get_images() const override
        {
                return m_images;
        }
        vec3f get_center() const override
        {
                return m_center;
        }
        float get_length() const override
        {
                return m_length;
        }

public:
        FileTxt(const std::string& file_name, ProgressRatio* progress);
};

void FileTxt::read_points_th(const ThreadData* thread_data, std::string* file_ptr, std::vector<size_t>* line_begin,
                             std::vector<vec3f>* lines, ProgressRatio* progress) const
{
        const size_t line_count = line_begin->size();
        const double line_count_d = line_begin->size();

        for (size_t line_num = thread_data->thread_num; line_num < line_count; line_num += thread_data->thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num / line_count_d);
                }

                size_t l_b = (*line_begin)[line_num];
                size_t l_e = (line_num + 1 < line_begin->size()) ? (*line_begin)[line_num + 1] : file_ptr->size();

                // В конце строки находится символ '\n', сместиться на него и записать вместо него 0
                --l_e;
                (*file_ptr)[l_e] = 0;

                read_float(*file_ptr, l_b, l_e, &(*lines)[line_num]);
        }
}

void FileTxt::read_points(const std::string& file_name, ProgressRatio* progress)
{
        int hardware_concurrency = get_hardware_concurrency();

        std::string file_str;
        std::vector<size_t> line_begin;
        get_file_lines(file_name, &file_str, &line_begin);
        m_vertices.resize(line_begin.size());

        std::vector<std::thread> threads(hardware_concurrency);
        std::vector<ThreadData> thread_data(threads.size());
        std::vector<std::string> thread_messages(threads.size());

        for (size_t i = 0; i < thread_data.size(); ++i)
        {
                ThreadData t;
                t.thread_num = i;
                t.thread_count = threads.size();
                thread_data[i] = t;
        }

        for (size_t i = 0; i < threads.size(); ++i)
        {
                launch_class_thread(&threads[i], &thread_messages[i], &FileTxt::read_points_th, this, &thread_data[i], &file_str,
                                    &line_begin, &m_vertices, progress);
        }

        join_threads(&threads, &thread_messages);
}

void FileTxt::read_text(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        read_points(file_name, progress);

        if (m_vertices.size() == 0)
        {
                error("No vertices found in Text file");
        }

        m_points.resize(m_vertices.size());
        std::iota(m_points.begin(), m_points.end(), 0);

        find_center_and_length(m_vertices, m_points, &m_center, &m_length);
}

FileTxt::FileTxt(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = get_time_seconds();

        read_text(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}
}

std::unique_ptr<IObj> load_obj_from_file(const std::string& file_name, ProgressRatio* progress)
{
        std::string upper_extension = to_upper(get_extension(file_name));

        if (upper_extension == "OBJ")
        {
                return std::make_unique<FileObj>(file_name, progress);
        }
        else if (upper_extension == "TXT")
        {
                return std::make_unique<FileTxt>(file_name, progress);
        }
        else
        {
                std::string msg = "Unsupported file format";
                if (upper_extension.size() > 0)
                {
                        msg += " " + get_extension(file_name);
                }

                error(msg);
        }
}
