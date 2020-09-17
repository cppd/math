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

#include "../bounding_box.h"
#include "../file_info.h"
#include "../normalize.h"
#include "../unique.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/utility/file/sys.h>
#include <src/utility/string/str.h>

#include <fstream>

namespace mesh::file
{
namespace
{
constexpr bool NORMALIZE_VERTEX_COORDINATES = false;

constexpr const char* OBJ_comment_and_space = "# ";
constexpr char OBJ_v = 'v';
constexpr char OBJ_n = 'n';
constexpr char OBJ_f = 'f';
constexpr char OBJ_l = 'l';

void write_comment(std::ostream& file, const std::string_view& comment)
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

        file << str;
}

template <size_t N>
void write_vertex(std::ostream& file, const Vector<N, float>& vertex)
{
        file << OBJ_v;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << vertex[i];
        }
        file << '\n';
}

template <size_t N>
void write_normal(std::ostream& file, const Vector<N, float>& normal)
{
        file << OBJ_v << OBJ_n;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << normal[i];
        }
        file << '\n';
}

template <size_t N>
void write_face(std::ostream& file, const std::array<int, N>& vertices)
{
        file << OBJ_f;
        for (unsigned i = 0; i < N; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                file << ' ' << (vertices[i] + 1);
        }
        file << '\n';
}

template <size_t N>
void write_face(std::ostream& file, const std::array<int, N>& vertices, const std::array<int, N>& normals)
{
        file << OBJ_f;
        for (unsigned i = 0; i < N; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                file << ' ' << (vertices[i] + 1) << "//" << (normals[i] + 1);
        }
        file << '\n';
}

void write_line(std::ostream& file, const std::array<int, 2>& vertices)
{
        file << OBJ_l;
        for (unsigned i = 0; i < 2; ++i)
        {
                // В файлах OBJ номера начинаются с 1
                file << ' ' << vertices[i] + 1;
        }
        file << '\n';
}

template <size_t N>
void write_vertices(std::ostream& file, const std::vector<Vector<N, float>>& vertices)
{
        for (const Vector<N, float>& v : vertices)
        {
                write_vertex(file, v);
        }
}

template <size_t N>
void write_vertices(std::ostream& file, const Mesh<N>& mesh)
{
        if (NORMALIZE_VERTEX_COORDINATES)
        {
                std::optional<mesh::BoundingBox<N>> box = mesh::bounding_box_by_facets_and_lines(mesh);
                if (!box)
                {
                        error("Facet and line coordinates are not found");
                }
                write_vertices(file, normalize_vertices(mesh, *box));
        }
        else
        {
                write_vertices(file, mesh.vertices);
        }
}

template <size_t N>
void write_normals(std::ostream& file, const Mesh<N>& mesh)
{
        for (const Vector<N, float>& vn : mesh.normals)
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
void write_facets(std::ostream& file, const Mesh<N>& mesh)
{
        // Вершины граней надо записывать в трёхмерный OBJ таким образом,
        // чтобы при обходе против часовой стрелки перпендикуляр к грани
        // был направлен противоположно направлению взгляда. В данной модели
        // нет перпендикуляров у граней, есть только у вершин, поэтому нужно
        // попытаться определить правильное направление по векторам вершин,
        // если у вершин они заданы.

        for (const typename Mesh<N>::Facet& f : mesh.facets)
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

                        Vector<3, double> v0 = to_vector<double>(mesh.vertices[v[0]]);
                        Vector<3, double> v1 = to_vector<double>(mesh.vertices[v[1]]);
                        Vector<3, double> v2 = to_vector<double>(mesh.vertices[v[2]]);

                        // Перпендикуляр к грани при обходе вершин против часовой
                        // стрелки и противоположно направлению взгляда
                        Vector<3, double> normal = cross(v1 - v0, v2 - v0);

                        // Если перпендикуляр в противоположном направлении со всеми
                        // векторами вершин, то меняется порядок вершин.
                        if (dot(to_vector<double>(mesh.normals[n[0]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[n[1]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[n[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                std::swap(n[1], n[2]);
                        }

                        write_face(file, v, n);
                }
        }
}

template <size_t N>
void write_lines(std::ostream& file, const Mesh<N>& mesh)
{
        for (const typename Mesh<N>::Line& l : mesh.lines)
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
                if (!is_obj_file_extension(N, e))
                {
                        error("Wrong " + obj_type_name(N) + " file name extension: " + e);
                }

                return file_name;
        }

        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + obj_file_extension(N);
}

template <size_t N>
void check_facets_and_lines(const Mesh<N>& mesh)
{
        if (mesh.facets.empty() && mesh.lines.empty())
        {
                error("Mesh has neither facets nor lines");
        }

        std::vector<int> facet_indices = unique_facet_indices(mesh);
        std::vector<int> line_indices = unique_line_indices(mesh);

        if (facet_indices.empty() && line_indices.empty())
        {
                error("Facet and line unique indices are not found");
        }
        if (!facet_indices.empty() && facet_indices.size() < N)
        {
                error("Facet unique indices count " + to_string(facet_indices.size()) + " is less than "
                      + to_string(N));
        }
        if (!line_indices.empty() && line_indices.size() < 2)
        {
                error("Line unique indices count " + to_string(line_indices.size()) + " is less than " + to_string(2));
        }
}
}

template <size_t N>
std::string save_to_obj_file(const Mesh<N>& mesh, const std::string& file_name, const std::string_view& comment)
{
        static_assert(N >= 3);

        check_facets_and_lines(mesh);

        std::string full_name = file_name_with_extension<N>(file_name);

        std::ofstream file(full_name);

        if (!file)
        {
                error("Error opening file for writing " + full_name);
        }

        file << std::scientific;
        file << std::setprecision(limits<float>::max_digits10);
        file << std::showpoint;

        double start_time = time_in_seconds();

        file << std::noshowpos;
        write_comment(file, comment);

        file << std::showpos;
        write_vertices(file, mesh);
        write_normals(file, mesh);

        file << std::noshowpos;
        write_facets(file, mesh);
        write_lines(file, mesh);

        if (!file)
        {
                error("Error writing to file " + full_name);
        }

        LOG(obj_type_name(N) + " saved, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return full_name;
}

template std::string save_to_obj_file(const Mesh<3>&, const std::string&, const std::string_view&);
template std::string save_to_obj_file(const Mesh<4>&, const std::string&, const std::string_view&);
template std::string save_to_obj_file(const Mesh<5>&, const std::string&, const std::string_view&);
template std::string save_to_obj_file(const Mesh<6>&, const std::string&, const std::string_view&);
}
