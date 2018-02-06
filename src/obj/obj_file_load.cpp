/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include "com/file/file_read.h"
#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/string/ascii.h"
#include "com/string/str.h"
#include "com/thread.h"
#include "com/time.h"
#include "com/types.h"

#include <SFML/Graphics/Image.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <thread>

constexpr int MAX_FACES_PER_LINE = 5;

constexpr const char OBJ_v[] = "v";
constexpr const char OBJ_vt[] = "vt";
constexpr const char OBJ_vn[] = "vn";
constexpr const char OBJ_f[] = "f";
constexpr const char OBJ_usemtl[] = "usemtl";
constexpr const char OBJ_mtllib[] = "mtllib";

constexpr const char MTL_newmtl[] = "newmtl";
constexpr const char MTL_Ka[] = "Ka";
constexpr const char MTL_Kd[] = "Kd";
constexpr const char MTL_Ks[] = "Ks";
constexpr const char MTL_Ns[] = "Ns";
constexpr const char MTL_map_Ka[] = "map_Ka";
constexpr const char MTL_map_Kd[] = "map_Kd";
constexpr const char MTL_map_Ks[] = "map_Ks";

constexpr bool is_number_sign(char c)
{
        return c == '#';
}
constexpr bool is_hyphen_minus(char c)
{
        return c == '-';
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

static_assert(str_equal("ab", "ab") && str_equal("", "") && !str_equal("", "ab") && !str_equal("ab", "") &&
              !str_equal("ab", "ac") && !str_equal("ba", "ca") && !str_equal("a", "xyz"));

namespace
{
template <typename Data, typename Op>
void read(const Data& data, long long size, const Op& op, long long* i)
{
        while (*i < size && op(data[*i]))
        {
                ++(*i);
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

template <typename T>
void find_line_begin(const T& s, std::vector<long long>* line_begin)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        long long size = s.size();

        long long new_line_count = 0;
        for (long long i = 0; i < size; ++i)
        {
                if (s[i] == '\n')
                {
                        ++new_line_count;
                }
        }

        line_begin->clear();
        line_begin->reserve(new_line_count);
        line_begin->shrink_to_fit();

        long long begin = 0;
        for (long long i = 0; i < size; ++i)
        {
                if (s[i] == '\n')
                {
                        line_begin->push_back(begin);
                        begin = i + 1;
                }
        }

        if (begin != size)
        {
                error("No new line at the end of file");
        }
}

template <typename T>
void read_file_lines(const std::string& file_name, T* file_data, std::vector<long long>* line_begin)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        read_text_file(file_name, file_data);

        find_line_begin(*file_data, line_begin);
}

IObj::Image read_image_from_file(const std::string& file_name)
{
        sf::Image image;
        if (!image.loadFromFile(file_name))
        {
                error("Error open image file " + file_name);
        }

        unsigned long long buffer_size = 4ull * image.getSize().x * image.getSize().y;

        IObj::Image obj_image;
        obj_image.dimensions[0] = image.getSize().x;
        obj_image.dimensions[1] = image.getSize().y;
        obj_image.srgba_pixels.resize(buffer_size);

        static_assert(sizeof(decltype(*obj_image.srgba_pixels.data())) == sizeof(decltype(*image.getPixelsPtr())));

        std::memcpy(obj_image.srgba_pixels.data(), image.getPixelsPtr(), buffer_size);

        return obj_image;
}

void load_image(const std::string& dir_name, const std::string& image_name, std::map<std::string, int>* image_index,
                std::vector<IObj::Image>* images, int* index)
{
        std::string file_name = trim(image_name);

        if (file_name.size() == 0)
        {
                error("No image file name");
        }

#if defined(__linux__)
        // путь к файлу может быть указан в формате Windows, поэтому надо заменить разделители
        std::replace(file_name.begin(), file_name.end(), '\\', '/');
#endif

        file_name = dir_name + "/" + file_name;

        if (auto iter = image_index->find(file_name); iter != image_index->end())
        {
                *index = iter->second;
                return;
        };

        images->push_back(read_image_from_file(file_name));
        *index = images->size() - 1;
        image_index->emplace(file_name, *index);
}

// Между begin и end находится уже проверенное целое число в формате DDDDD без знака
template <typename Integer, typename T>
Integer digits_to_integer(const T& data, long long begin, long long end)
{
        static_assert(is_native_integral<Integer>);

        long long length = end - begin;

        if (length > limits<Integer>::digits10 || length < 1)
        {
                error("Error convert " + std::string(&data[begin], length) + " to integral");
        }

        --end;
        Integer sum = ASCII::char_to_int(data[end]);
        Integer mul = 1;
        while (--end >= begin)
        {
                mul *= 10;
                sum += ASCII::char_to_int(data[end]) * mul;
        }

        return sum;
}

template <typename T, typename Integer>
[[nodiscard]] bool read_integer(const T& data, long long size, long long* pos, Integer* value)
{
        static_assert(is_signed<Integer>);

        long long begin = *pos;

        if (begin < size && is_hyphen_minus(data[begin]))
        {
                ++begin;
        }

        long long end = begin;

        read(data, size, ASCII::is_digit, &end);

        if (end > begin)
        {
                *value = (begin == *pos) ? digits_to_integer<Integer>(data, begin, end) :
                                           -digits_to_integer<Integer>(data, begin, end);
                *pos = end;

                return true;
        }

        return false;
}

// Варианты данных: "x/x/x ...", "x//x ...", "x// ...", "x/x/ ...", "x/x ...", "x ...".
template <typename T, size_t MaxGroupCount, size_t GroupSize, typename IndexType>
void read_digit_groups(const T& line, long long begin, long long end,
                       std::array<std::array<IndexType, GroupSize>, MaxGroupCount>* group_ptr, int* group_count)
{
        int group_index = -1;

        long long i = begin;

        while (true)
        {
                ++group_index;

                read(line, end, ASCII::is_space, &i);

                if (i == end)
                {
                        *group_count = group_index;
                        return;
                }

                if (group_index >= static_cast<int>(group_ptr->size()))
                {
                        error("Found too many face vertices " + to_string(group_index + 1) +
                              " (max supported = " + to_string(group_ptr->size()) + ")");
                }

                std::array<IndexType, GroupSize>& indices = (*group_ptr)[group_index];

                // Считывается номер вершины
                if (read_integer(line, end, &i, &indices[0]))
                {
                        if (indices[0] == 0)
                        {
                                error("Zero face index");
                        }
                }
                else
                {
                        error("Error read face vertex first number");
                }

                // Считываются текстура и нормаль
                for (int a = 1; a < static_cast<int>(indices.size()); ++a)
                {
                        if (i == end || ASCII::is_space(line[i]))
                        {
                                indices[a] = 0;
                                continue;
                        }

                        if (!is_solidus(line[i]))
                        {
                                error("Error read face vertex number");
                        }

                        ++i;

                        if (i == end || ASCII::is_space(line[i]))
                        {
                                indices[a] = 0;
                                continue;
                        }

                        if (read_integer(line, end, &i, &indices[a]))
                        {
                                if (indices[a] == 0)
                                {
                                        error("Zero face index");
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
// Индексы находятся в порядке face, texture, normal.
template <typename T, size_t N>
void check_index_consistent(const std::array<std::array<T, 3>, N>& groups, int group_count)
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
                error("Inconsistent face texture indices");
        }

        if (!(normal == 0 || normal == group_count))
        {
                error("Inconsistent face normal indices");
        }
}

template <typename T>
void read_faces(const T& data, long long begin, long long end, std::array<IObj::Face, MAX_FACES_PER_LINE>* faces, int* face_count)

{
        constexpr int MAX_GROUP_COUNT = MAX_FACES_PER_LINE + 2;

        std::array<std::array<int, 3>, MAX_GROUP_COUNT> groups;

        int group_count;

        read_digit_groups(data, begin, end, &groups, &group_count);

        if (group_count < 3)
        {
                error("Error face vertex count " + to_string(group_count) + " (min = 3)");
        }

        // Обязательная проверка индексов
        check_index_consistent(groups, group_count);

        *face_count = group_count - 2;

        for (int i = 0; i < *face_count; ++i)
        {
                (*faces)[i].has_texcoord = !(groups[0][1] == 0);
                (*faces)[i].has_normal = !(groups[0][2] == 0);

                (*faces)[i].vertices[0] = groups[0][0];
                (*faces)[i].texcoords[0] = groups[0][1];
                (*faces)[i].normals[0] = groups[0][2];

                (*faces)[i].vertices[1] = groups[i + 1][0];
                (*faces)[i].texcoords[1] = groups[i + 1][1];
                (*faces)[i].normals[1] = groups[i + 1][2];

                (*faces)[i].vertices[2] = groups[i + 2][0];
                (*faces)[i].texcoords[2] = groups[i + 2][1];
                (*faces)[i].normals[2] = groups[i + 2][2];
        }
}

template <typename T>
bool read_float(const char** str, T* p)
{
        using FP = std::remove_volatile_t<T>;

        static_assert(std::is_same_v<FP, float> || std::is_same_v<FP, double> || std::is_same_v<FP, long double>);

        char* end;

        if constexpr (std::is_same_v<FP, float>)
        {
                *p = std::strtof(*str, &end);
        }
        if constexpr (std::is_same_v<FP, double>)
        {
                *p = std::strtod(*str, &end);
        }
        if constexpr (std::is_same_v<FP, long double>)
        {
                *p = std::strtold(*str, &end);
        }

        // В соответствии со спецификацией файла OBJ, между числами должны быть пробелы,
        // а после чисел пробелы, конец строки или комментарий.
        // Здесь без проверок этого.
        if (*str == end || errno == ERANGE || !is_finite(*p))
        {
                return false;
        }
        else
        {
                *str = end;
                return true;
        }
};

template <typename... T>
int string_to_float(const char* str, T*... floats)
{
        constexpr int N = sizeof...(T);

        static_assert(N > 0);
        static_assert(((std::is_same_v<std::remove_volatile_t<T>, float> || std::is_same_v<std::remove_volatile_t<T>, double> ||
                        std::is_same_v<std::remove_volatile_t<T>, long double>)&&...));

        errno = 0;
        int cnt = 0;

        ((read_float(&str, floats) ? ++cnt : false) && ...);

        return cnt;
}

void read_float(const char* str, vec3f* v)
{
        if (3 != string_to_float(str, &(*v)[0], &(*v)[1], &(*v)[2]))
        {
                error("Error read 3 floating points");
        }
}

void read_float_texture(const char* str, vec2f* v)
{
        float tmp;

        int n = string_to_float(str, &(*v)[0], &(*v)[1], &tmp);
        if (n != 2 && n != 3)
        {
                error("Error read 2 or 3 floating points");
        }
        if (n == 3 && tmp != 0.0f)
        {
                error("3D textures not supported");
        }
}

void read_float(const char* str, float* v)
{
        if (1 != string_to_float(str, v))
        {
                error("Error read 1 floating point");
        }
}

template <typename T>
void read_name(const char* object_name, const T& data, long long begin, long long end, std::string* name)
{
        const long long size = end;

        long long i = begin;
        read(data, size, ASCII::is_space, &i);
        if (i == size)
        {
                error("Error read " + std::string(object_name) + " name");
        }

        long long i2 = i;
        read(data, size, ASCII::is_not_space, &i2);

        *name = std::string(&data[i], i2 - i);

        i = i2;

        read(data, size, ASCII::is_space, &i);
        if (i != size)
        {
                error("Error read " + std::string(object_name) + " name");
        }
}

template <typename T>
void read_library_names(const T& data, long long begin, long long end, std::vector<std::string>* v,
                        std::set<std::string>* lib_unique_names)
{
        const long long size = end;
        bool found = false;
        long long i = begin;

        while (true)
        {
                read(data, size, ASCII::is_space, &i);
                if (i == size)
                {
                        if (!found)
                        {
                                error("Library name not found");
                        }
                        return;
                }

                long long i2 = i;
                read(data, size, ASCII::is_not_space, &i2);
                std::string name = std::string(&data[i], i2 - i);
                i = i2;
                found = true;

                if (lib_unique_names->find(name) == lib_unique_names->end())
                {
                        v->push_back(name);
                        lib_unique_names->insert(std::move(name));
                }
        }
}

// Разделение строки на 2 части " не_пробелы | остальной текст до символа комментария или конца строки"
template <typename T, typename Space, typename Comment>
void split(const T& data, long long first, long long last, const Space& space, const Comment& comment, long long* first_b,
           long long* first_e, long long* second_b, long long* second_e)
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
void split_line(T* data, const std::vector<long long>& line_begin, long long line_num, const char** first, const char** second,
                long long* second_b, long long* second_e)
{
        long long line_count = line_begin.size();

        long long last = (line_num + 1 < line_count) ? line_begin[line_num + 1] : data->size();

        // В конце строки находится символ '\n', сместиться на него
        --last;

        long long first_b, first_e;

        split(*data, line_begin[line_num], last, ASCII::is_space, is_number_sign, &first_b, &first_e, second_b, second_e);

        *first = &(*data)[first_b];
        (*data)[first_e] = 0; // пробел, символ комментария '#' или символ '\n'

        *second = &(*data)[*second_b];
        (*data)[*second_e] = 0; // символ комментария '#' или символ '\n'
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
        std::vector<Face> m_faces;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        vec3f m_center;
        float m_length;

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

        // enum class MtlLineType
        //{
        //        NEWMTL,
        //        KA,
        //        KD,
        //        KS,
        //        NS,
        //        MAP_KA,
        //        MAP_KD,
        //        MAP_KS,
        //        NONE,
        //        NOT_SUPPORTED
        //};

        struct ObjLine
        {
                ObjLineType type;
                long long second_b;
                long long second_e;
                std::array<Face, MAX_FACES_PER_LINE> faces;
                int face_count;
                vec3f v;
        };

        // struct MtlLine
        //{
        //        MtlLineType type;
        //        long long second_b, second_e;
        //        vec3f v;
        //};

        struct Counters
        {
                int vertex = 0;
                int texcoord = 0;
                int normal = 0;
                int face = 0;

                void operator+=(const Counters& counters)
                {
                        vertex += counters.vertex;
                        texcoord += counters.texcoord;
                        normal += counters.normal;
                        face += counters.face;
                }
        };

        void check_face_indices() const;

        bool remove_one_dimensional_faces();

        static void read_obj_stage_one(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters,
                                       std::vector<char>* data_ptr, std::vector<long long>* line_begin,
                                       std::vector<ObjLine>* line_prop, ProgressRatio* progress);

        void read_obj_stage_two(const Counters& counters, std::vector<char>* data_ptr, std::vector<ObjLine>* line_prop,
                                ProgressRatio* progress, std::map<std::string, int>* material_index,
                                std::vector<std::string>* library_names);

        Counters sum_counters(const std::vector<Counters>& counters);

        void read_obj_thread(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters, ThreadBarrier* barrier,
                             std::atomic_bool* error_found, std::vector<char>* data_ptr, std::vector<long long>* line_begin,
                             std::vector<ObjLine>* line_prop, ProgressRatio* progress, std::map<std::string, int>* material_index,
                             std::vector<std::string>* library_names);

        void read_obj(const std::string& file_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                      std::vector<std::string>* library_names);

        void read_lib(const std::string& dir_name, const std::string& file_name, ProgressRatio* progress,
                      std::map<std::string, int>* material_index, std::map<std::string, int>* image_index);

        void read_libs(const std::string& dir_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                       const std::vector<std::string>& library_names);

        void read_obj_and_mtl(const std::string& file_name, ProgressRatio* progress);

        const std::vector<vec3f>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Face>& faces() const override
        {
                return m_faces;
        }
        const std::vector<Point>& points() const override
        {
                return m_points;
        }
        const std::vector<Line>& lines() const override
        {
                return m_lines;
        }
        const std::vector<Material>& materials() const override
        {
                return m_materials;
        }
        const std::vector<Image>& images() const override
        {
                return m_images;
        }
        vec3f center() const override
        {
                return m_center;
        }
        float length() const override
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

        for (const Face& face : m_faces)
        {
                for (int i = 0; i < 3; ++i)
                {
                        if (face.vertices[i] < 0 || face.vertices[i] >= vertex_count)
                        {
                                error("Vertex index " + std::to_string(face.vertices[i]) + " is out of bounds [0, " +
                                      std::to_string(vertex_count) + ")");
                        }

                        if (face.has_texcoord)
                        {
                                if (face.texcoords[i] < 0 || face.texcoords[i] >= texcoord_count)
                                {
                                        error("Texture coordinate index " + std::to_string(face.texcoords[i]) +
                                              " is out of bounds [0, " + std::to_string(texcoord_count) + ")");
                                }
                        }
                        else
                        {
                                if (face.texcoords[i] != -1)
                                {
                                        error("No texture but texture coordinate index is not set to -1");
                                }
                        }

                        if (face.has_normal)
                        {
                                if (face.normals[i] < 0 || face.normals[i] >= normal_count)
                                {
                                        error("Normal index " + std::to_string(face.normals[i]) + " is out of bounds [0, " +
                                              std::to_string(normal_count) + ")");
                                }
                        }
                        else
                        {
                                if (face.normals[i] != -1)
                                {
                                        error("No normals but normal coordinate index is not set to -1");
                                }
                        }
                }
        }
}

bool FileObj::remove_one_dimensional_faces()
{
        std::vector<bool> one_d_faces(m_faces.size(), false);

        int one_d_face_count = 0;

        for (size_t i = 0; i < m_faces.size(); ++i)
        {
                vec3f v0 = m_vertices[m_faces[i].vertices[0]];
                vec3f v1 = m_vertices[m_faces[i].vertices[1]];
                vec3f v2 = m_vertices[m_faces[i].vertices[2]];

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

        std::vector<Face> faces;
        faces.reserve(m_faces.size() - one_d_face_count);

        for (size_t i = 0; i < m_faces.size(); ++i)
        {
                if (!one_d_faces[i])
                {
                        faces.push_back(m_faces[i]);
                }
        }

        m_faces = std::move(faces);

        return true;
}

void FileObj::read_obj_stage_one(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters,
                                 std::vector<char>* data_ptr, std::vector<long long>* line_begin, std::vector<ObjLine>* line_prop,
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

                ObjLine lp;

                const char* first;
                const char* second;

                split_line(&data, *line_begin, line_num, &first, &second, &lp.second_b, &lp.second_e);

                try
                {
                        if (str_equal(first, OBJ_v))
                        {
                                lp.type = ObjLineType::V;
                                vec3f v;
                                read_float(&data[lp.second_b], &v);
                                lp.v = v;

                                ++((*counters)[thread_num].vertex);
                        }
                        else if (str_equal(first, OBJ_vt))
                        {
                                lp.type = ObjLineType::VT;
                                vec2f v;
                                read_float_texture(&data[lp.second_b], &v);
                                lp.v[0] = v[0];
                                lp.v[1] = v[1];

                                ++((*counters)[thread_num].texcoord);
                        }
                        else if (str_equal(first, OBJ_vn))
                        {
                                lp.type = ObjLineType::VN;
                                vec3f v;
                                read_float(&data[lp.second_b], &v);
                                lp.v = normalize(v);

                                ++((*counters)[thread_num].normal);
                        }
                        else if (str_equal(first, OBJ_f))
                        {
                                lp.type = ObjLineType::F;
                                read_faces(data, lp.second_b, lp.second_e, &lp.faces, &lp.face_count);

                                ++((*counters)[thread_num].face);
                        }
                        else if (str_equal(first, OBJ_usemtl))
                        {
                                lp.type = ObjLineType::USEMTL;
                        }
                        else if (str_equal(first, OBJ_mtllib))
                        {
                                lp.type = ObjLineType::MTLLIB;
                        }
                        else if (!*first)
                        {
                                lp.type = ObjLineType::NONE;
                        }
                        else
                        {
                                lp.type = ObjLineType::NOT_SUPPORTED;
                        }
                }
                catch (std::exception& e)
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
void correct_indices(IObj::Face* face, int vertices_size, int texcoords_size, int normals_size)
{
        for (int i = 0; i < 3; ++i)
        {
                int& v = face->vertices[i];
                int& t = face->texcoords[i];
                int& n = face->normals[i];

                if (v == 0)
                {
                        error("Correct indices vertex index is zero");
                }

                v = v > 0 ? v - 1 : vertices_size + v;
                t = t > 0 ? t - 1 : (t < 0 ? texcoords_size + t : -1);
                n = n > 0 ? n - 1 : (n < 0 ? normals_size + n : -1);
        }
}

void FileObj::read_obj_stage_two(const Counters& counters, std::vector<char>* data_ptr, std::vector<ObjLine>* line_prop,
                                 ProgressRatio* progress, std::map<std::string, int>* material_index,
                                 std::vector<std::string>* library_names)
{
        m_vertices.reserve(counters.vertex);
        m_texcoords.reserve(counters.texcoord);
        m_normals.reserve(counters.normal);
        m_faces.reserve(counters.face);

        const std::vector<char>& data = *data_ptr;
        const long long line_count = line_prop->size();
        const double line_count_reciprocal = 1.0 / line_prop->size();

        int mtl_index = -1;
        std::string mtl_name;
        std::set<std::string> unique_library_names;

        for (long long line_num = 0; line_num < line_count; ++line_num)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
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
                        for (int i = 0; i < lp.face_count; ++i)
                        {
                                lp.faces[i].material = mtl_index;
                                correct_indices(&lp.faces[i], m_vertices.size(), m_texcoords.size(), m_normals.size());
                                m_faces.push_back(std::move(lp.faces[i]));
                        }
                        break;
                case ObjLineType::USEMTL:
                {
                        read_name("material", data, lp.second_b, lp.second_e, &mtl_name);
                        auto iter = material_index->find(mtl_name);
                        if (iter != material_index->end())
                        {
                                mtl_index = iter->second;
                        }
                        else
                        {
                                IObj::Material mtl;
                                mtl.name = mtl_name;
                                m_materials.push_back(std::move(mtl));
                                material_index->emplace(std::move(mtl_name), m_materials.size() - 1);
                                mtl_index = m_materials.size() - 1;
                        }
                        break;
                }
                case ObjLineType::MTLLIB:
                        read_library_names(data, lp.second_b, lp.second_e, library_names, &unique_library_names);
                        break;
                case ObjLineType::NONE:
                        break;
                case ObjLineType::NOT_SUPPORTED:
                        break;
                }
        }
}

FileObj::Counters FileObj::sum_counters(const std::vector<Counters>& counters)
{
        Counters sum;
        for (const Counters& c : counters)
        {
                sum += c;
        }
        return sum;
}

void FileObj::read_obj_thread(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters, ThreadBarrier* barrier,
                              std::atomic_bool* error_found, std::vector<char>* data_ptr, std::vector<long long>* line_begin,
                              std::vector<ObjLine>* line_prop, ProgressRatio* progress,
                              std::map<std::string, int>* material_index, std::vector<std::string>* library_names)
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

        read_obj_stage_two(sum_counters(*counters), data_ptr, line_prop, progress, material_index, library_names);
}

void FileObj::read_lib(const std::string& dir_name, const std::string& file_name, ProgressRatio* progress,
                       std::map<std::string, int>* material_index, std::map<std::string, int>* image_index)
{
        std::vector<char> data;
        std::vector<long long> line_begin;

        const std::string lib_name = dir_name + "/" + file_name;

        read_file_lines(lib_name, &data, &line_begin);

        const std::string lib_dir = get_dir_name(lib_name);

        FileObj::Material* mtl = nullptr;
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
                                continue;
                        }
                        else if (str_equal(first, MTL_newmtl))
                        {
                                if (material_index->size() == 0)
                                {
                                        // все материалы найдены
                                        break;
                                }

                                read_name("material", data, second_b, second_e, &name);

                                auto iter = material_index->find(name);
                                if (iter != material_index->end())
                                {
                                        mtl = &(m_materials[iter->second]);
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
                                read_float(&data[second_b], &mtl->Ka);

                                if (!check_range(mtl->Ka, 0, 1))
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
                                read_float(&data[second_b], &mtl->Kd);

                                if (!check_range(mtl->Kd, 0, 1))
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
                                read_float(&data[second_b], &mtl->Ks);

                                if (!check_range(mtl->Ks, 0, 1))
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
                                load_image(lib_dir, name, image_index, &m_images, &mtl->map_Ka);
                        }
                        else if (str_equal(first, MTL_map_Kd))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }
                                read_name("file", data, second_b, second_e, &name);
                                load_image(lib_dir, name, image_index, &m_images, &mtl->map_Kd);
                        }
                        else if (str_equal(first, MTL_map_Ks))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }
                                read_name("file", data, second_b, second_e, &name);
                                load_image(lib_dir, name, image_index, &m_images, &mtl->map_Ks);
                        }
                }
                catch (std::exception& e)
                {
                        error("Library: " + lib_name + "\n" + "Line " + to_string(line_num) + ": " + first + " " + second + "\n" +
                              e.what());
                }
                catch (...)
                {
                        error("Library: " + lib_name + "\n" + "Line " + to_string(line_num) + ": " + first + " " + second + "\n" +
                              "Unknown error");
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
        const int hardware_concurrency = get_hardware_concurrency();

        std::vector<char> data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &data, &line_begin);

        std::vector<ObjLine> line_prop(line_begin.size());
        ThreadBarrier barrier(hardware_concurrency);
        std::atomic_bool error_found{false};
        std::vector<Counters> counters(hardware_concurrency);

        ThreadsWithCatch threads(hardware_concurrency);
        for (int i = 0; i < hardware_concurrency; ++i)
        {
                threads.add([&, i]() {
                        read_obj_thread(i, hardware_concurrency, &counters, &barrier, &error_found, &data, &line_begin,
                                        &line_prop, progress, material_index, library_names);
                });
        }
        threads.join();
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

        center_and_length(m_vertices, m_faces, &m_center, &m_length);

        if (remove_one_dimensional_faces())
        {
                if (m_faces.size() == 0)
                {
                        error("No 2D faces found in OBJ file");
                }
                center_and_length(m_vertices, m_faces, &m_center, &m_length);
        }

        read_libs(get_dir_name(file_name), progress, &material_index, library_names);
}

FileObj::FileObj(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        read_obj_and_mtl(file_name, progress);

        LOG("OBJ loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}

// Чтение вершин из текстового файла. Одна вершина на строку. Три координаты через пробел.
// x y z
// x y z
class FileTxt final : public IObj
{
        std::vector<vec3f> m_vertices;
        std::vector<vec2f> m_texcoords;
        std::vector<vec3f> m_normals;
        std::vector<Face> m_faces;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        vec3f m_center;
        float m_length;

        void read_points_thread(unsigned thread_num, unsigned thread_count, std::vector<char>* data_ptr,
                                const std::vector<long long>& line_begin, std::vector<vec3f>* lines,
                                ProgressRatio* progress) const;
        void read_points(const std::string& file_name, ProgressRatio* progress);
        void read_text(const std::string& file_name, ProgressRatio* progress);

        const std::vector<vec3f>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<vec2f>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<vec3f>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Face>& faces() const override
        {
                return m_faces;
        }
        const std::vector<Point>& points() const override
        {
                return m_points;
        }
        const std::vector<Line>& lines() const override
        {
                return m_lines;
        }
        const std::vector<Material>& materials() const override
        {
                return m_materials;
        }
        const std::vector<Image>& images() const override
        {
                return m_images;
        }
        vec3f center() const override
        {
                return m_center;
        }
        float length() const override
        {
                return m_length;
        }

public:
        FileTxt(const std::string& file_name, ProgressRatio* progress);
};

void FileTxt::read_points_thread(unsigned thread_num, unsigned thread_count, std::vector<char>* data_ptr,
                                 const std::vector<long long>& line_begin, std::vector<vec3f>* lines,
                                 ProgressRatio* progress) const
{
        const long long line_count = line_begin.size();
        const double line_count_reciprocal = 1.0 / line_begin.size();

        for (long long line_num = thread_num; line_num < line_count; line_num += thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                const char* str = &(*data_ptr)[line_begin[line_num]];

                long long last = (line_num < line_count - 1) ? line_begin[line_num + 1] : data_ptr->size();

                // В конце строки находится символ '\n', сместиться на него и записать вместо него 0
                --last;

                (*data_ptr)[last] = 0;

                try
                {
                        read_float(str, &(*lines)[line_num]);
                }
                catch (std::exception& e)
                {
                        error("Line " + to_string(line_num) + ": " + str + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line_num) + ": " + str + "\n" + "Unknown error");
                }
        }
}

void FileTxt::read_points(const std::string& file_name, ProgressRatio* progress)
{
        const int hardware_concurrency = get_hardware_concurrency();

        std::vector<char> file_data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &file_data, &line_begin);

        m_vertices.resize(line_begin.size());

        ThreadsWithCatch threads(hardware_concurrency);
        for (int i = 0; i < hardware_concurrency; ++i)
        {
                threads.add(
                        [&, i]() { read_points_thread(i, hardware_concurrency, &file_data, line_begin, &m_vertices, progress); });
        }
        threads.join();
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
        for (unsigned i = 0; i < m_points.size(); ++i)
        {
                m_points[i].vertex = i;
        }

        center_and_length(m_vertices, m_points, &m_center, &m_length);
}

FileTxt::FileTxt(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        read_text(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}
}

std::unique_ptr<IObj> load_obj_from_file(const std::string& file_name, ProgressRatio* progress)
{
        std::string upper_extension = to_upper(get_extension(file_name));

        if (upper_extension == "OBJ")
        {
                return std::make_unique<FileObj>(file_name, progress);
        }

        if (upper_extension == "TXT")
        {
                return std::make_unique<FileTxt>(file_name, progress);
        }

        std::string ext = get_extension(file_name);
        if (ext.size() > 0)
        {
                error("Unsupported file format " + ext);
        }
        else
        {
                error("File extension not found");
        }
}
