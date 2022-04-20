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

#include "save_stl.h"

#include "../bounding_box.h"
#include "../file_info.h"
#include "../unique.h"
#include "../vertices.h"

#include <src/com/chrono.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/numerical/complement.h>

#include <bit>
#include <fstream>

namespace ns::mesh::file
{
namespace
{
constexpr bool NORMALIZE_VERTEX_COORDINATES = false;

static_assert(std::endian::native == std::endian::little, "Binary STL numbers must be little-endian");

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
        file << "solid " << solid_name << '\n';
}

void write_end_ascii(std::ostream& file, const std::string& solid_name)
{
        file << "endsolid " << solid_name << '\n';
}

void write_begin_binary(std::ostream& file, unsigned facet_count)
{
        struct Begin final
        {
                std::array<std::uint8_t, 80> header;
                std::uint32_t number_of_triangles;
        };

        static_assert(sizeof(Begin) == 84 * sizeof(std::uint8_t));

        Begin begin;
        std::memset(begin.header.data(), 0, begin.header.size());
        begin.number_of_triangles = facet_count;
        file.write(reinterpret_cast<const char*>(&begin), sizeof(begin));
}

void write_end_binary(std::ostream& file)
{
        struct End final
        {
                std::uint16_t attribute_byte_count;
        };

        static_assert(sizeof(End) == 2 * sizeof(std::uint8_t));

        End end;
        end.attribute_byte_count = 0;
        file.write(reinterpret_cast<const char*>(&end), sizeof(end));
}

template <bool ASCII, std::size_t N>
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

        if (ASCII)
        {
                // clang-format off
                static constexpr std::string_view FACET_NORMAL = "facet normal";
                static constexpr std::string_view OUTER_LOOP   = "  outer loop";
                static constexpr std::string_view VERTEX       = "    vertex";
                static constexpr std::string_view END_LOOP     = "  endloop";
                static constexpr std::string_view END_FACET    = "endfacet";
                // clang-format on

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

template <bool ASCII, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh, const std::vector<Vector<N, float>>& vertices)
{
        for (const typename Mesh<N>::Facet& f : mesh.facets)
        {
                if (!f.has_normal)
                {
                        Vector<N, double> normal =
                                numerical::orthogonal_complement<N, float, double>(vertices, f.vertices);
                        write_facet<ASCII>(file, normal, f.vertices, vertices);
                }
                else if constexpr (N != 3)
                {
                        Vector<N, double> normal =
                                numerical::orthogonal_complement<N, float, double>(vertices, f.vertices);
                        write_facet<ASCII>(file, normal, f.vertices, vertices);
                }
                else
                {
                        std::array<int, 3> v = f.vertices;

                        Vector<3, double> v0 = to_vector<double>(mesh.vertices[v[0]]);
                        Vector<3, double> v1 = to_vector<double>(mesh.vertices[v[1]]);
                        Vector<3, double> v2 = to_vector<double>(mesh.vertices[v[2]]);

                        Vector<3, double> normal = cross(v1 - v0, v2 - v0);

                        if (dot(to_vector<double>(mesh.normals[f.normals[0]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[1]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                normal = -normal;
                        }

                        write_facet<ASCII>(file, normal, v, vertices);
                }
        }
}

template <bool ASCII, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh)
{
        if (NORMALIZE_VERTEX_COORDINATES)
        {
                std::optional<mesh::BoundingBox<N>> box = mesh::bounding_box_by_facets(mesh);
                if (!box)
                {
                        error("Facet coordinates are not found");
                }
                write_facets<ASCII>(file, mesh, normalize_vertices(mesh, *box));
        }
        else
        {
                write_facets<ASCII>(file, mesh, mesh.vertices);
        }
}

std::string stl_type_name(std::size_t n)
{
        return "STL-" + to_string(n);
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

template <std::size_t N, typename Path>
std::filesystem::path save_to_stl_file(
        const Mesh<N>& mesh,
        const Path& file_name,
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
                file << std::setprecision(Limits<float>::max_digits10());
                file << std::showpoint;
                file << std::showpos;
        }

        const Clock::time_point start_time = Clock::now();

        write(ascii_format, file, mesh, comment);

        if (!file)
        {
                error("Error writing to file " + generic_utf8_filename(full_name));
        }

        LOG(stl_type_name(N) + " saved, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return full_name;
}

#define SAVE_TO_STL_FILE_INSTANTIATION(N)                \
        template std::filesystem::path save_to_stl_file( \
                const Mesh<(N)>&, const std::filesystem::path&, const std::string_view&, bool);

SAVE_TO_STL_FILE_INSTANTIATION(3)
SAVE_TO_STL_FILE_INSTANTIATION(4)
SAVE_TO_STL_FILE_INSTANTIATION(5)
SAVE_TO_STL_FILE_INSTANTIATION(6)
}
