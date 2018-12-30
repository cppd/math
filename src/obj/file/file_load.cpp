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

#include "file_load.h"

#include "obj_file.h"

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
#include "com/type_name.h"
#include "com/types.h"
#include "obj/alg/alg.h"

#include <SFML/Graphics/Image.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <thread>

template <size_t N>
constexpr int MAX_FACETS_PER_LINE = 1;
template <>
constexpr int MAX_FACETS_PER_LINE<3> = 5;

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
std::string obj_type_name(size_t N)
{
        return "OBJ-" + to_string(N);
}

template <typename Data, typename Op>
void read(const Data& data, long long size, const Op& op, long long* i)
{
        while (*i < size && op(data[*i]))
        {
                ++(*i);
        }
}

template <typename T>
std::string map_keys_to_string(const std::map<std::string, T>& m)
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

template <typename T1, typename T2, typename T3>
bool check_range(const T1& v, const T2& min, const T3& max)
{
        return v >= min && v <= max;
}

bool check_color(const Color& v)
{
        return v.red() >= 0 && v.red() <= 1 && v.green() >= 0 && v.green() <= 1 && v.blue() >= 0 && v.blue() <= 1;
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

template <size_t N>
typename Obj<N>::Image read_image_from_file(const std::string& file_name)
{
        if constexpr (N != 3)
        {
                error("Reading " + to_string(N - 1) + "-dimensional images for " + obj_type_name(N) + " is not supported");
        }
        else
        {
                sf::Image image;

                if (!image.loadFromFile(file_name))
                {
                        error("Error load image from file " + file_name);
                }

                image.flipVertically();

                unsigned long long buffer_size = 4ull * image.getSize().x * image.getSize().y;

                Obj<3>::Image obj_image;

                obj_image.size[0] = image.getSize().x;
                obj_image.size[1] = image.getSize().y;
                obj_image.srgba_pixels.resize(buffer_size);

                static_assert(sizeof(decltype(*obj_image.srgba_pixels.data())) == sizeof(decltype(*image.getPixelsPtr())));

                std::memcpy(obj_image.srgba_pixels.data(), image.getPixelsPtr(), buffer_size);

                return obj_image;
        }
}

template <size_t N>
void load_image(const std::string& dir_name, const std::string& image_name, std::map<std::string, int>* image_index,
                std::vector<typename Obj<N>::Image>* images, int* index)
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

        images->push_back(read_image_from_file<N>(file_name));
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
        Integer sum = ascii::char_to_int(data[end]);
        Integer mul = 1;
        while (--end >= begin)
        {
                mul *= 10;
                sum += ascii::char_to_int(data[end]) * mul;
        }

        return sum;
}

template <typename T, typename Integer>
bool read_integer(const T& data, long long size, long long* pos, Integer* value)
{
        static_assert(is_signed<Integer>);

        long long begin = *pos;

        if (begin < size && is_hyphen_minus(data[begin]))
        {
                ++begin;
        }

        long long end = begin;

        read(data, size, ascii::is_digit, &end);

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

                read(line, end, ascii::is_space, &i);

                if (i == end)
                {
                        *group_count = group_index;
                        return;
                }

                if (group_index >= static_cast<int>(group_ptr->size()))
                {
                        error("Found too many facet vertices " + to_string(group_index + 1) +
                              " (max supported = " + to_string(group_ptr->size()) + ")");
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
void read_facets(const T& data, long long begin, long long end,
                 std::array<typename Obj<N>::Facet, MAX_FACETS_PER_LINE<N>>* facets, int* facet_count)
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

template <typename T>
bool read_one_float_from_string(const char** str, T* p)
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
int string_to_floats(const char* str, T*... floats)
{
        constexpr int N = sizeof...(T);

        static_assert(N > 0);
        static_assert(((std::is_same_v<std::remove_volatile_t<T>, float> || std::is_same_v<std::remove_volatile_t<T>, double> ||
                        std::is_same_v<std::remove_volatile_t<T>, long double>)&&...));

        errno = 0;
        int cnt = 0;

        ((read_one_float_from_string(&str, floats) ? ++cnt : false) && ...);

        return cnt;
}

template <size_t N, typename T, unsigned... I>
int read_vector(const char* str, Vector<N, T>* v, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));

        return string_to_floats(str, &(*v)[I]...);
}

template <size_t N, typename T, unsigned... I>
int read_vector(const char* str, Vector<N, T>* v, T* n, std::integer_sequence<unsigned, I...>&&)
{
        static_assert(N == sizeof...(I));

        return string_to_floats(str, &(*v)[I]..., n);
}

template <size_t N, typename T>
void read_float(const char* str, Vector<N, T>* v)
{
        if (N != read_vector(str, v, std::make_integer_sequence<unsigned, N>()))
        {
                error(std::string("Error read " + to_string(N) + " floating points of ") + type_name<T>() + " type");
        }
}

template <size_t N, typename T>
void read_float_texture(const char* str, Vector<N, T>* v)
{
        T tmp;

        int n = read_vector(str, v, &tmp, std::make_integer_sequence<unsigned, N>());

        if (n != N && n != N + 1)
        {
                error(std::string("Error read " + to_string(N) + " or " + to_string(N + 1) + " floating points of ") +
                      type_name<T>() + " type");
        }

        if (n == N + 1 && tmp != 0)
        {
                error(to_string(N + 1) + "-dimensional textures are not supported");
        }
}

template <typename T>
void read_float(const char* str, T* v)
{
        static_assert(std::is_floating_point_v<T>);

        if (1 != string_to_floats(str, v))
        {
                error(std::string("Error read 1 floating point of ") + type_name<T>() + " type");
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
void read_library_names(const T& data, long long begin, long long end, std::vector<std::string>* v,
                        std::set<std::string>* lib_unique_names)
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

        split(*data, line_begin[line_num], last, ascii::is_space, is_number_sign, &first_b, &first_e, second_b, second_e);

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

template <size_t N>
class FileObj final : public Obj<N>
{
        using typename Obj<N>::Facet;
        using typename Obj<N>::Point;
        using typename Obj<N>::Line;
        using typename Obj<N>::Material;
        using typename Obj<N>::Image;

        std::vector<Vector<N, float>> m_vertices;
        std::vector<Vector<N, float>> m_normals;
        std::vector<Vector<N - 1, float>> m_texcoords;
        std::vector<Facet> m_facets;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        Vector<N, float> m_center;
        float m_length;

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

        struct ObjLine
        {
                ObjLineType type;
                long long second_b;
                long long second_e;
                std::array<Facet, MAX_FACETS_PER_LINE<N>> facets;
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

        void check_facet_indices() const;

        bool remove_facets_with_incorrect_dimension();

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

        const std::vector<Vector<N, float>>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<Vector<N, float>>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Vector<N - 1, float>>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<Facet>& facets() const override
        {
                return m_facets;
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
        Vector<N, float> center() const override
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

template <size_t N>
void FileObj<N>::check_facet_indices() const
{
        int vertex_count = m_vertices.size();
        int texcoord_count = m_texcoords.size();
        int normal_count = m_normals.size();

        for (const Facet& facet : m_facets)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (facet.vertices[i] < 0 || facet.vertices[i] >= vertex_count)
                        {
                                error("Vertex index " + to_string(facet.vertices[i]) + " is out of bounds [0, " +
                                      to_string(vertex_count) + ")");
                        }

                        if (facet.has_texcoord)
                        {
                                if (facet.texcoords[i] < 0 || facet.texcoords[i] >= texcoord_count)
                                {
                                        error("Texture coordinate index " + to_string(facet.texcoords[i]) +
                                              " is out of bounds [0, " + to_string(texcoord_count) + ")");
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
                                        error("Normal index " + to_string(facet.normals[i]) + " is out of bounds [0, " +
                                              to_string(normal_count) + ")");
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
bool FileObj<N>::remove_facets_with_incorrect_dimension()
{
        if constexpr (N != 3)
        {
                return false;
        }
        else
        {
                std::vector<bool> wrong_facets(m_facets.size(), false);

                int wrong_facet_count = 0;

                for (size_t i = 0; i < m_facets.size(); ++i)
                {
                        if (!facet_dimension_is_correct(m_vertices, m_facets[i].vertices))
                        {
                                wrong_facets[i] = true;
                                ++wrong_facet_count;
                        }
                }

                if (wrong_facet_count == 0)
                {
                        return false;
                }

                std::vector<Facet> facets;
                facets.reserve(m_facets.size() - wrong_facet_count);

                for (size_t i = 0; i < m_facets.size(); ++i)
                {
                        if (!wrong_facets[i])
                        {
                                facets.push_back(m_facets[i]);
                        }
                }

                m_facets = std::move(facets);

                return true;
        }
}

template <size_t N>
void FileObj<N>::read_obj_stage_one(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters,
                                    std::vector<char>* data_ptr, std::vector<long long>* line_begin,
                                    std::vector<ObjLine>* line_prop, ProgressRatio* progress)
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
                                lp.v = normalize(v);

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
template <size_t N>
void correct_indices(typename Obj<N>::Facet* facet, int vertices_size, int texcoords_size, int normals_size)
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
void FileObj<N>::read_obj_stage_two(const Counters& counters, std::vector<char>* data_ptr, std::vector<ObjLine>* line_prop,
                                    ProgressRatio* progress, std::map<std::string, int>* material_index,
                                    std::vector<std::string>* library_names)
{
        m_vertices.reserve(counters.vertex);
        m_texcoords.reserve(counters.texcoord);
        m_normals.reserve(counters.normal);
        m_facets.reserve(counters.facet);

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
                case ObjLineType::v:
                        m_vertices.push_back(lp.v);
                        break;
                case ObjLineType::vt:
                {
                        m_texcoords.resize(m_texcoords.size() + 1);
                        Vector<N - 1, float>& new_vector = m_texcoords[m_texcoords.size() - 1];
                        for (unsigned i = 0; i < N - 1; ++i)
                        {
                                new_vector[i] = lp.v[i];
                        }
                        break;
                }
                case ObjLineType::vn:
                        m_normals.push_back(lp.v);
                        break;
                case ObjLineType::f:
                        for (int i = 0; i < lp.facet_count; ++i)
                        {
                                lp.facets[i].material = mtl_index;
                                correct_indices<N>(&lp.facets[i], m_vertices.size(), m_texcoords.size(), m_normals.size());
                                m_facets.push_back(std::move(lp.facets[i]));
                        }
                        break;
                case ObjLineType::usemtl:
                {
                        read_name("material", data, lp.second_b, lp.second_e, &mtl_name);
                        auto iter = material_index->find(mtl_name);
                        if (iter != material_index->end())
                        {
                                mtl_index = iter->second;
                        }
                        else
                        {
                                typename Obj<N>::Material mtl;
                                mtl.name = mtl_name;
                                m_materials.push_back(std::move(mtl));
                                material_index->emplace(std::move(mtl_name), m_materials.size() - 1);
                                mtl_index = m_materials.size() - 1;
                        }
                        break;
                }
                case ObjLineType::mtllib:
                        read_library_names(data, lp.second_b, lp.second_e, library_names, &unique_library_names);
                        break;
                case ObjLineType::None:
                        break;
                case ObjLineType::NotSupported:
                        break;
                }
        }
}

template <size_t N>
typename FileObj<N>::Counters FileObj<N>::sum_counters(const std::vector<Counters>& counters)
{
        Counters sum;
        for (const Counters& c : counters)
        {
                sum += c;
        }
        return sum;
}

template <size_t N>
void FileObj<N>::read_obj_thread(unsigned thread_num, unsigned thread_count, std::vector<Counters>* counters,
                                 ThreadBarrier* barrier, std::atomic_bool* error_found, std::vector<char>* data_ptr,
                                 std::vector<long long>* line_begin, std::vector<ObjLine>* line_prop, ProgressRatio* progress,
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

template <size_t N>
void FileObj<N>::read_lib(const std::string& dir_name, const std::string& file_name, ProgressRatio* progress,
                          std::map<std::string, int>* material_index, std::map<std::string, int>* image_index)
{
        std::vector<char> data;
        std::vector<long long> line_begin;

        const std::string lib_name = dir_name + "/" + file_name;

        read_file_lines(lib_name, &data, &line_begin);

        const std::string lib_dir = file_parent_path(lib_name);

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
                                load_image<N>(lib_dir, name, image_index, &m_images, &mtl->map_Ka);
                        }
                        else if (str_equal(first, MTL_map_Kd))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_name("file", data, second_b, second_e, &name);
                                load_image<N>(lib_dir, name, image_index, &m_images, &mtl->map_Kd);
                        }
                        else if (str_equal(first, MTL_map_Ks))
                        {
                                if (!mtl)
                                {
                                        continue;
                                }

                                read_name("file", data, second_b, second_e, &name);
                                load_image<N>(lib_dir, name, image_index, &m_images, &mtl->map_Ks);
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

template <size_t N>
void FileObj<N>::read_libs(const std::string& dir_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                           const std::vector<std::string>& library_names)
{
        std::map<std::string, int> image_index;

        for (size_t i = 0; (i < library_names.size()) && (material_index->size() > 0); ++i)
        {
                read_lib(dir_name, library_names[i], progress, material_index, &image_index);
        }

        if (material_index->size() != 0)
        {
                error("Materials not found in libraries: " + map_keys_to_string(*material_index));
        }

        m_materials.shrink_to_fit();
        m_images.shrink_to_fit();
}

template <size_t N>
void FileObj<N>::read_obj(const std::string& file_name, ProgressRatio* progress, std::map<std::string, int>* material_index,
                          std::vector<std::string>* library_names)
{
        const int thread_count = hardware_concurrency();

        std::vector<char> data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &data, &line_begin);

        std::vector<ObjLine> line_prop(line_begin.size());
        ThreadBarrier barrier(thread_count);
        std::atomic_bool error_found{false};
        std::vector<Counters> counters(thread_count);

        ThreadsWithCatch threads(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
                threads.add([&, i]() {
                        read_obj_thread(i, thread_count, &counters, &barrier, &error_found, &data, &line_begin, &line_prop,
                                        progress, material_index, library_names);
                });
        }
        threads.join();
}

template <size_t N>
void FileObj<N>::read_obj_and_mtl(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        std::map<std::string, int> material_index;
        std::vector<std::string> library_names;

        read_obj(file_name, progress, &material_index, &library_names);

        if (m_facets.size() == 0)
        {
                error("No facets found in OBJ file");
        }

        check_facet_indices();

        center_and_length(m_vertices, m_facets, &m_center, &m_length);

        if (remove_facets_with_incorrect_dimension())
        {
                if (m_facets.size() == 0)
                {
                        error("No " + to_string(N - 1) + "-facets found in " + obj_type_name(N) + " file");
                }
                center_and_length(m_vertices, m_facets, &m_center, &m_length);
        }

        read_libs(file_parent_path(file_name), progress, &material_index, library_names);
}

template <size_t N>
FileObj<N>::FileObj(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        read_obj_and_mtl(file_name, progress);

        LOG(obj_type_name(N) + " loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}

// Чтение вершин из текстового файла. Одна вершина на строку. Координаты через пробел.
// x0 x1 x2 x3 ...
// x0 x1 x2 x3 ...
template <size_t N>
class FileTxt final : public Obj<N>
{
        using typename Obj<N>::Facet;
        using typename Obj<N>::Point;
        using typename Obj<N>::Line;
        using typename Obj<N>::Material;
        using typename Obj<N>::Image;

        std::vector<Vector<N, float>> m_vertices;
        std::vector<Vector<N, float>> m_normals;
        std::vector<Vector<N - 1, float>> m_texcoords;
        std::vector<Facet> m_facets;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        Vector<N, float> m_center;
        float m_length;

        void read_points_thread(unsigned thread_num, unsigned thread_count, std::vector<char>* data_ptr,
                                const std::vector<long long>& line_begin, std::vector<Vector<N, float>>* lines,
                                ProgressRatio* progress) const;
        void read_points(const std::string& file_name, ProgressRatio* progress);
        void read_text(const std::string& file_name, ProgressRatio* progress);

        const std::vector<Vector<N, float>>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<Vector<N, float>>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Vector<N - 1, float>>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<Facet>& facets() const override
        {
                return m_facets;
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
        Vector<N, float> center() const override
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

template <size_t N>
void FileTxt<N>::read_points_thread(unsigned thread_num, unsigned thread_count, std::vector<char>* data_ptr,
                                    const std::vector<long long>& line_begin, std::vector<Vector<N, float>>* lines,
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

template <size_t N>
void FileTxt<N>::read_points(const std::string& file_name, ProgressRatio* progress)
{
        const int thread_count = hardware_concurrency();

        std::vector<char> file_data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &file_data, &line_begin);

        m_vertices.resize(line_begin.size());

        ThreadsWithCatch threads(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
                threads.add([&, i]() { read_points_thread(i, thread_count, &file_data, line_begin, &m_vertices, progress); });
        }
        threads.join();
}

template <size_t N>
void FileTxt<N>::read_text(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        read_points(file_name, progress);

        if (m_vertices.size() == 0)
        {
                error("No vertices found in TXT file");
        }

        m_points.resize(m_vertices.size());
        for (unsigned i = 0; i < m_points.size(); ++i)
        {
                m_points[i].vertex = i;
        }

        center_and_length(m_vertices, m_points, &m_center, &m_length);
}

template <size_t N>
FileTxt<N>::FileTxt(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        read_text(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}
}

template <size_t N>
std::unique_ptr<Obj<N>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress)
{
        auto [obj_dimension, obj_file_type] = obj_file_dimension_and_type(file_name);

        if (obj_dimension != static_cast<int>(N))
        {
                error("Requested OBJ file dimension " + to_string(N) + ", detected OBJ file dimension " +
                      to_string(obj_dimension) + ", file " + file_name);
        }

        switch (obj_file_type)
        {
        case ObjFileType::Obj:
                return std::make_unique<FileObj<N>>(file_name, progress);
        case ObjFileType::Txt:
                return std::make_unique<FileTxt<N>>(file_name, progress);
        }

        error_fatal("Unknown obj file type");
}

template std::unique_ptr<Obj<3>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<4>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<5>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Obj<6>> load_obj_from_file(const std::string& file_name, ProgressRatio* progress);
