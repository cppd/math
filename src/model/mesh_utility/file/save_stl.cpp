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

#include "save_stl.h"

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
#include <src/numerical/orthogonal.h>

#include <bit>
#include <fstream>

namespace ns::mesh::file
{
namespace
{
constexpr bool NORMALIZE_VERTEX_COORDINATES = false;

static_assert(std::endian::native == std::endian::little, "Binary STL numbers must be little-endian");

// clang-format off
constexpr const char* SOLID        = "solid";
constexpr const char* FACET_NORMAL = "facet normal";
constexpr const char* OUTER_LOOP   = "  outer loop";
constexpr const char* VERTEX       = "    vertex";
constexpr const char* END_LOOP     = "  endloop";
constexpr const char* END_FACET    = "endfacet";
constexpr const char* END_SOLID    = "endsolid";
// clang-format on

std::string comment_to_solid_name(const std::string_view& comment)
{
        std::string str;
        for (char c : comment)
        {
                str += (c != '\n') ? c : ' ';
        }
        str = trim(str);
        if (!str.empty())
        {
                return str;
        }
        return "s";
}

void write_begin_ascii(std::ostream& file, const std::string& solid_name)
{
        file << SOLID << ' ' << solid_name << '\n';
}

void write_end_ascii(std::ostream& file, const std::string& solid_name)
{
        file << END_SOLID << ' ' << solid_name << '\n';
}

void write_begin_binary(std::ostream& file, unsigned facet_count)
{
        struct Begin
        {
                std::array<uint8_t, 80> header;
                uint32_t number_of_triangles;
        };
        static_assert(sizeof(Begin) == 84 * sizeof(uint8_t));
        Begin begin;
        std::memset(begin.header.data(), 0, begin.header.size());
        begin.number_of_triangles = facet_count;
        file.write(reinterpret_cast<const char*>(&begin), sizeof(begin));
}

void write_end_binary(std::ostream& file)
{
        struct End
        {
                uint16_t attribute_byte_count;
        };
        static_assert(sizeof(End) == 2 * sizeof(uint8_t));
        End end;
        end.attribute_byte_count = 0;
        file.write(reinterpret_cast<const char*>(&end), sizeof(end));
}

template <bool ascii, std::size_t N>
void write_facet(
        std::ostream& file,
        const Vector<N, double>& normal,
        const std::array<int, N>& indices,
        const std::vector<Vector<N, float>>& vertices)
{
        Vector<N, float> n = to_vector<float>(normal.normalized());
        if (!is_finite(n))
        {
                n = Vector<N, float>(0);
        }

        if (ascii)
        {
                file << FACET_NORMAL;
                for (unsigned i = 0; i < N; ++i)
                {
                        file << ' ' << n[i];
                }
                file << '\n';

                file << OUTER_LOOP << '\n';
                for (unsigned i = 0; i < N; ++i)
                {
                        file << VERTEX;
                        for (unsigned j = 0; j < N; ++j)
                        {
                                file << ' ' << vertices[indices[i]][j];
                        }
                        file << '\n';
                }
                file << END_LOOP << '\n';
                file << END_FACET << '\n';
        }
        else
        {
                static_assert(sizeof(n) == N * sizeof(float));
                file.write(reinterpret_cast<const char*>(&n), sizeof(n));
                for (unsigned i = 0; i < N; ++i)
                {
                        static_assert(sizeof(vertices[indices[i]]) == N * sizeof(float));
                        file.write(reinterpret_cast<const char*>(&vertices[indices[i]]), sizeof(vertices[indices[i]]));
                }
        }
}

template <bool ascii, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh, const std::vector<Vector<N, float>>& vertices)
{
        // Вершины граней надо записывать в трёхмерный STL таким образом,
        // чтобы при обходе против часовой стрелки перпендикуляр к грани
        // был направлен противоположно направлению взгляда. В данной модели
        // нет перпендикуляров у граней, есть только у вершин, поэтому нужно
        // попытаться определить правильное направление по векторам вершин,
        // если у вершин они заданы.

        for (const typename Mesh<N>::Facet& f : mesh.facets)
        {
                if (!f.has_normal)
                {
                        Vector<N, double> normal = ortho_nn<N, float, double>(vertices, f.vertices);
                        write_facet<ascii>(file, normal, f.vertices, vertices);
                }
                else if constexpr (N != 3)
                {
                        Vector<N, double> normal = ortho_nn<N, float, double>(vertices, f.vertices);
                        write_facet<ascii>(file, normal, f.vertices, vertices);
                }
                else
                {
                        std::array<int, 3> v = f.vertices;

                        Vector<3, double> v0 = to_vector<double>(mesh.vertices[v[0]]);
                        Vector<3, double> v1 = to_vector<double>(mesh.vertices[v[1]]);
                        Vector<3, double> v2 = to_vector<double>(mesh.vertices[v[2]]);

                        // Перпендикуляр к грани при обходе вершин против часовой
                        // стрелки и противоположно направлению взгляда
                        Vector<3, double> normal = cross(v1 - v0, v2 - v0);

                        // Если перпендикуляр в противоположном направлении со всеми
                        // векторами вершин, то меняется порядок вершин.
                        if (dot(to_vector<double>(mesh.normals[f.normals[0]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[1]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                normal = -normal;
                        }

                        write_facet<ascii>(file, normal, v, vertices);
                }
        }
}

template <bool ascii, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh)
{
        if (NORMALIZE_VERTEX_COORDINATES)
        {
                std::optional<mesh::BoundingBox<N>> box = mesh::bounding_box_by_facets(mesh);
                if (!box)
                {
                        error("Facet coordinates are not found");
                }
                write_facets<ascii>(file, mesh, normalize_vertices(mesh, *box));
        }
        else
        {
                write_facets<ascii>(file, mesh, mesh.vertices);
        }
}

std::string stl_type_name(std::size_t N)
{
        return "STL-" + to_string(N);
}

template <std::size_t N>
std::filesystem::path file_name_with_extension(std::filesystem::path file_name)
{
        if (file_name.has_extension())
        {
                if (!file_has_stl_extension(N, file_name))
                {
                        error("Wrong " + stl_type_name(N)
                              + " file name extension: " + generic_utf8_filename(file_name));
                }

                return file_name;
        }

        return file_name.replace_extension(path_from_utf8(stl_file_extension(N)));
}

template <std::size_t N>
void check_facets(const Mesh<N>& mesh)
{
        if (mesh.facets.empty())
        {
                error("Mesh has no facets");
        }

        std::vector<int> facet_indices = unique_facet_indices(mesh);

        if (facet_indices.empty())
        {
                error("Facet unique indices are not found");
        }
        if (!facet_indices.empty() && facet_indices.size() < N)
        {
                error("Facet unique indices count " + to_string(facet_indices.size()) + " is less than "
                      + to_string(N));
        }
}

template <std::size_t N>
void write(bool ascii, std::ostream& file, const Mesh<N>& mesh, const std::string_view& comment)
{
        if (ascii)
        {
                const std::string solid_name = comment_to_solid_name(comment);
                write_begin_ascii(file, solid_name);
                write_facets<true>(file, mesh);
                write_end_ascii(file, solid_name);
        }
        else
        {
                write_begin_binary(file, mesh.facets.size());
                write_facets<false>(file, mesh);
                write_end_binary(file);
        }
}
}

template <std::size_t N>
std::filesystem::path save_to_stl_file(
        const Mesh<N>& mesh,
        const std::filesystem::path& file_name,
        const std::string_view& comment,
        bool ascii_format)
{
        static_assert(N >= 3);

        check_facets(mesh);

        std::filesystem::path full_name = file_name_with_extension<N>(file_name);

        std::ofstream file(full_name);

        if (!file)
        {
                error("Error opening file for writing " + generic_utf8_filename(full_name));
        }

        if (ascii_format)
        {
                file << std::scientific;
                file << std::setprecision(limits<float>::max_digits10);
                file << std::showpoint;
                file << std::showpos;
        }

        TimePoint start_time = time();

        write(ascii_format, file, mesh, comment);

        if (!file)
        {
                error("Error writing to file " + generic_utf8_filename(full_name));
        }

        LOG(stl_type_name(N) + " saved, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return full_name;
}

template std::filesystem::path save_to_stl_file(
        const Mesh<3>&,
        const std::filesystem::path&,
        const std::string_view&,
        bool);
template std::filesystem::path save_to_stl_file(
        const Mesh<4>&,
        const std::filesystem::path&,
        const std::string_view&,
        bool);
template std::filesystem::path save_to_stl_file(
        const Mesh<5>&,
        const std::filesystem::path&,
        const std::string_view&,
        bool);
template std::filesystem::path save_to_stl_file(
        const Mesh<6>&,
        const std::filesystem::path&,
        const std::string_view&,
        bool);
}
