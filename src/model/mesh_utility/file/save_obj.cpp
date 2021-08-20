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

#include "save_obj.h"

#include "../bounding_box.h"
#include "../file_info.h"
#include "../unique.h"
#include "../vertices.h"

#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>

#include <fstream>

namespace ns::mesh::file
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

template <std::size_t N>
void write_vertex(std::ostream& file, const Vector<N, float>& vertex)
{
        file << OBJ_v;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << vertex[i];
        }
        file << '\n';
}

template <std::size_t N>
void write_normal(std::ostream& file, const Vector<N, float>& normal)
{
        file << OBJ_v << OBJ_n;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << normal[i];
        }
        file << '\n';
}

template <std::size_t N>
void write_face(std::ostream& file, const std::array<int, N>& vertices)
{
        file << OBJ_f;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << (vertices[i] + 1);
        }
        file << '\n';
}

template <std::size_t N>
void write_face(std::ostream& file, const std::array<int, N>& vertices, const std::array<int, N>& normals)
{
        file << OBJ_f;
        for (unsigned i = 0; i < N; ++i)
        {
                file << ' ' << (vertices[i] + 1) << "//" << (normals[i] + 1);
        }
        file << '\n';
}

void write_line(std::ostream& file, const std::array<int, 2>& vertices)
{
        file << OBJ_l;
        for (unsigned i = 0; i < 2; ++i)
        {
                file << ' ' << (vertices[i] + 1);
        }
        file << '\n';
}

template <std::size_t N>
void write_vertices(std::ostream& file, const std::vector<Vector<N, float>>& vertices)
{
        for (const Vector<N, float>& v : vertices)
        {
                write_vertex(file, v);
        }
}

template <std::size_t N>
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

template <std::size_t N>
void write_normals(std::ostream& file, const Mesh<N>& mesh)
{
        for (const Vector<N, float>& vn : mesh.normals)
        {
                Vector<N, float> normal = to_vector<float>(to_vector<double>(vn).normalized());
                if (!is_finite(normal))
                {
                        normal = Vector<N, float>(0);
                }
                write_normal(file, normal);
        }
}

template <std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh)
{
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

                        Vector<3, double> normal = cross(v1 - v0, v2 - v0);

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

template <std::size_t N>
void write_lines(std::ostream& file, const Mesh<N>& mesh)
{
        for (const typename Mesh<N>::Line& l : mesh.lines)
        {
                write_line(file, l.vertices);
        }
}

std::string obj_type_name(std::size_t n)
{
        return "OBJ-" + to_string(n);
}

template <std::size_t N>
std::filesystem::path file_name_with_extension(std::filesystem::path file_name)
{
        if (file_name.has_extension())
        {
                if (!file_has_obj_extension(N, file_name))
                {
                        error("Wrong " + obj_type_name(N)
                              + " file name extension: " + generic_utf8_filename(file_name));
                }

                return file_name;
        }

        return file_name.replace_extension(path_from_utf8(obj_file_extension(N)));
}

template <std::size_t N>
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

template <std::size_t N, typename Path>
std::filesystem::path save_to_obj_file(const Mesh<N>& mesh, const Path& file_name, const std::string_view& comment)
{
        static_assert(N >= 3);

        check_facets_and_lines(mesh);

        std::filesystem::path full_name = file_name_with_extension<N>(file_name);

        std::ofstream file(full_name);

        if (!file)
        {
                error("Error opening file for writing " + generic_utf8_filename(full_name));
        }

        file << std::scientific;
        file << std::setprecision(limits<float>::max_digits10);
        file << std::showpoint;

        TimePoint start_time = time();

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
                error("Error writing to file " + generic_utf8_filename(full_name));
        }

        LOG(obj_type_name(N) + " saved, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return full_name;
}

#define SAVE_TO_OBJ_FILE_INSTANTIATION(N)                \
        template std::filesystem::path save_to_obj_file( \
                const Mesh<(N)>&, const std::filesystem::path&, const std::string_view&);

SAVE_TO_OBJ_FILE_INSTANTIATION(3)
SAVE_TO_OBJ_FILE_INSTANTIATION(4)
SAVE_TO_OBJ_FILE_INSTANTIATION(5)
SAVE_TO_OBJ_FILE_INSTANTIATION(6)
}
