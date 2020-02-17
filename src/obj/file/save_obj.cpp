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

#include "save_obj.h"

#include "../alg/alg.h"
#include "../file.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/util/file/file.h>
#include <src/util/file/sys.h>
#include <src/util/string/str.h>

constexpr const char* OBJ_comment_and_space = "# ";
constexpr const char* OBJ_v = "v";
constexpr const char* OBJ_vn = "vn";
constexpr const char* OBJ_f = "f";
constexpr const char* OBJ_l = "l";

namespace
{
void write_comment(const CFile& file, const std::string_view& comment)
{
        if (comment.empty())
        {
                return;
        }

        std::string str;

        str += OBJ_comment_and_space;
        for (char c : comment)
        {
                if (c != '\n')
                {
                        str += c;
                }
                else
                {
                        str += '\n';
                        str += OBJ_comment_and_space;
                }
        }
        str += '\n';

        fprintf(file, "%s", str.c_str());
}

template <size_t N, typename T>
void write_vector(const CFile& file, const Vector<N, T>& vector)
{
        static_assert(limits<float>::max_digits10 <= 9);
        static_assert(limits<double>::max_digits10 <= 17);
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        constexpr const char* format = (std::is_same_v<T, float>) ? " %12.9f" : " %20.17f";

        for (unsigned i = 0; i < N; ++i)
        {
                fprintf(file, format, vector[i]);
        }
}

template <size_t N>
void write_vertex(const CFile& file, const Vector<N, float>& vertex)
{
        fprintf(file, "%s", OBJ_v);
        write_vector(file, vertex);
        fprintf(file, "\n");
}

template <size_t N>
void write_normal(const CFile& file, const Vector<N, float>& normal)
{
        fprintf(file, "%s", OBJ_vn);
        write_vector(file, normal);
        fprintf(file, "\n");
}

template <size_t N>
void write_face(const CFile& file, const std::array<int, N>& vertices)
{
        fprintf(file, "%s", OBJ_f);
        for (unsigned i = 0; i < N; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                long v = vertices[i] + 1;
                fprintf(file, " %ld", v);
        }
        fprintf(file, "\n");
}

template <size_t N>
void write_face(const CFile& file, const std::array<int, N>& vertices, const std::array<int, N>& normals)
{
        fprintf(file, "%s", OBJ_f);
        for (unsigned i = 0; i < N; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                long v = vertices[i] + 1;
                long n = normals[i] + 1;
                fprintf(file, " %ld//%ld", v, n);
        }
        fprintf(file, "\n");
}

void write_line(const CFile& file, const std::array<int, 2>& vertices)
{
        fprintf(file, "%s", OBJ_l);
        for (unsigned i = 0; i < 2; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                long v = vertices[i] + 1;
                fprintf(file, " %ld", v);
        }
        fprintf(file, "\n");
}

// Запись вершин с приведением координат вершин к интервалу [-1, 1] с сохранением пропорций
template <size_t N>
void write_vertices(const CFile& file, const Obj<N>* obj)
{
        std::vector<int> facet_indices = unique_facet_indices(obj);
        std::vector<int> line_indices = unique_line_indices(obj);

        if (facet_indices.empty() && line_indices.empty())
        {
                error("Facet and line unique indices are not found");
        }
        if (!facet_indices.empty() && facet_indices.size() < N)
        {
                error("Facet unique indices count " + to_string(facet_indices.size()) + " is less than " +
                      to_string(N));
        }
        if (!line_indices.empty() && line_indices.size() < 2)
        {
                error("Line unique indices count " + to_string(line_indices.size()) + " is less than " + to_string(2));
        }

        Vector<N, float> min;
        Vector<N, float> max;

        std::tie(min, max) = min_max_coordinates(obj->vertices(), facet_indices, line_indices);

        Vector<N, float> delta = max - min;

        float max_delta = delta.norm_infinity();

        if (max_delta == 0)
        {
                error("Object vertices are equal to each other");
        }

        float scale_factor = 2 / max_delta;
        Vector<N, float> center = min + 0.5f * delta;

        for (const Vector<N, float>& v : obj->vertices())
        {
                Vector<N, float> vertex = (v - center) * scale_factor;

                write_vertex(file, vertex);
        }
}

template <size_t N>
void write_normals(const CFile& file, const Obj<N>* obj)
{
        for (const Vector<N, float>& vn : obj->normals())
        {
                Vector<N, double> normal = to_vector<double>(vn);
                double len = normal.norm();

                if (len == 0)
                {
                        error("Object zero length normal");
                }

                normal /= len;

                write_normal(file, to_vector<float>(normal));
        }
}

template <size_t N>
void write_facets(const CFile& file, const Obj<N>* obj)
{
        // Вершины граней надо записывать в трёхмерный OBJ таким образом,
        // чтобы при обходе против часовой стрелки перпендикуляр к грани
        // был направлен противоположно направлению взгляда. В данной модели
        // нет перпендикуляров у граней, есть только у вершин, поэтому нужно
        // попытаться определить правильное направление по векторам вершин,
        // если у вершин они заданы.

        for (const typename Obj<N>::Facet& f : obj->facets())
        {
                if (!f.has_normal)
                {
                        write_face(file, f.vertices);
                }
                else if constexpr (N != 3)
                {
                        write_face(file, f.vertices, f.normals);
                }
                else
                {
                        std::array<int, 3> v = f.vertices;
                        std::array<int, 3> n = f.normals;

                        Vector<3, double> v0 = to_vector<double>(obj->vertices()[v[0]]);
                        Vector<3, double> v1 = to_vector<double>(obj->vertices()[v[1]]);
                        Vector<3, double> v2 = to_vector<double>(obj->vertices()[v[2]]);

                        // Перпендикуляр к грани при обходе вершин против часовой
                        // стрелки и противоположно направлению взгляда
                        Vector<3, double> normal = cross(v1 - v0, v2 - v0);

                        // Если перпендикуляр в противоположном направлении со всеми
                        // векторами вершин, то меняется порядок вершин.
                        if (dot(to_vector<double>(obj->normals()[n[0]]), normal) < 0 &&
                            dot(to_vector<double>(obj->normals()[n[1]]), normal) < 0 &&
                            dot(to_vector<double>(obj->normals()[n[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                std::swap(n[1], n[2]);
                        }

                        write_face(file, v, n);
                }
        }
}

template <size_t N>
void write_lines(const CFile& file, const Obj<N>* obj)
{
        for (const typename Obj<N>::Line& l : obj->lines())
        {
                write_line(file, l.vertices);
        }
}

std::string obj_type_name(size_t N)
{
        return "OBJ-" + to_string(N);
}

template <size_t N>
std::string file_name_with_extension(const std::string& file_name)
{
        std::string e = file_extension(file_name);

        if (!e.empty())
        {
                if (!obj_file_extension_is_correct(N, e))
                {
                        error("Wrong " + obj_type_name(N) + " file name extension: " + e);
                }

                return file_name;
        }

        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + obj_file_extension(N);
}
}

template <size_t N>
std::string save_obj(const Obj<N>* obj, const std::string& file_name, const std::string_view& comment)
{
        static_assert(N >= 3);

        if (obj->facets().empty() && obj->lines().empty())
        {
                error("Object has neither facets nor lines");
        }

        std::string full_name = file_name_with_extension<N>(file_name);

        CFile file(full_name, "w");

        double start_time = time_in_seconds();

        write_comment(file, comment);
        write_vertices(file, obj);
        write_normals(file, obj);
        write_facets(file, obj);
        write_lines(file, obj);

        LOG(obj_type_name(N) + " saved, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return full_name;
}

template std::string save_obj(const Obj<3>* obj, const std::string& file_name, const std::string_view& comment);
template std::string save_obj(const Obj<4>* obj, const std::string& file_name, const std::string_view& comment);
template std::string save_obj(const Obj<5>* obj, const std::string& file_name, const std::string_view& comment);
template std::string save_obj(const Obj<6>* obj, const std::string& file_name, const std::string_view& comment);
