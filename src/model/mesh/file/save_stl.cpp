/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "stl/swap.h"

#include <src/com/chrono.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/str.h>
#include <src/com/type/limit.h>
#include <src/model/mesh.h>
#include <src/model/mesh/bounding_box.h>
#include <src/model/mesh/file_info.h>
#include <src/model/mesh/unique.h>
#include <src/model/mesh/vertices.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace ns::model::mesh::file
{
namespace
{
constexpr bool NORMALIZE_VERTEX_COORDINATES = false;

std::string comment_to_solid_name(const std::string_view comment)
{
        std::string str;
        for (const char c : comment)
        {
                str += (c != '\n') ? c : ' ';
        }
        str = trim(str);
        if (str.empty())
        {
                str = "s";
        }
        return str;
}

void write_begin_ascii(std::ostream& file, const std::string& solid_name)
{
        file << "solid " << solid_name << '\n';
}

void write_end_ascii(std::ostream& file, const std::string& solid_name)
{
        file << "endsolid " << solid_name << '\n';
}

template <typename T>
void write(std::ostream& file, const T& data)
{
        file.write(reinterpret_cast<const char*>(data_pointer(data)), data_size(data));
}

template <bool BYTE_SWAP>
void write_begin_binary(std::ostream& file, const std::uint32_t facet_count)
{
        struct Begin final
        {
                std::array<std::uint8_t, 80> header;
                std::uint32_t number_of_triangles;
        };

        static_assert(sizeof(Begin) == 84 * sizeof(std::uint8_t));

        Begin begin;

        std::memset(begin.header.data(), 0, begin.header.size());
        if constexpr (!BYTE_SWAP)
        {
                begin.number_of_triangles = facet_count;
        }
        else
        {
                begin.number_of_triangles = std::byteswap(facet_count);
        }

        write(file, begin);
}

void write_end_binary(std::ostream& file)
{
        struct End final
        {
                std::uint16_t attribute_byte_count = 0;
        };

        static_assert(sizeof(End) == 2 * sizeof(std::uint8_t));

        write(file, End());
}

template <std::size_t N>
void write_ascii_facet(
        std::ostream& file,
        const numerical::Vector<N, float>& n,
        const std::array<int, N>& indices,
        const std::vector<numerical::Vector<N, float>>& vertices)
{
        // clang-format off
        static constexpr std::string_view FACET_NORMAL = "facet normal";
        static constexpr std::string_view OUTER_LOOP   = "  outer loop";
        static constexpr std::string_view VERTEX       = "    vertex";
        static constexpr std::string_view END_LOOP     = "  endloop";
        static constexpr std::string_view END_FACET    = "endfacet";
        // clang-format on

        file << FACET_NORMAL;
        for (std::size_t i = 0; i < N; ++i)
        {
                file << ' ' << n[i];
        }
        file << '\n';

        file << OUTER_LOOP << '\n';
        for (std::size_t i = 0; i < N; ++i)
        {
                file << VERTEX;
                for (std::size_t j = 0; j < N; ++j)
                {
                        file << ' ' << vertices[indices[i]][j];
                }
                file << '\n';
        }
        file << END_LOOP << '\n';
        file << END_FACET << '\n';
}

template <bool BYTE_SWAP, std::size_t N>
void write_binary_facet(
        std::ostream& file,
        const numerical::Vector<N, float>& n,
        const std::array<int, N>& indices,
        const std::vector<numerical::Vector<N, float>>& vertices)
{
        if constexpr (!BYTE_SWAP)
        {
                write(file, n);
                for (std::size_t i = 0; i < N; ++i)
                {
                        write(file, vertices[indices[i]]);
                }
        }
        else
        {
                write(file, stl::byte_swap(n));
                for (std::size_t i = 0; i < N; ++i)
                {
                        write(file, stl::byte_swap(vertices[indices[i]]));
                }
        }
}

template <bool ASCII, bool BYTE_SWAP, std::size_t N>
void write_facet(
        std::ostream& file,
        const numerical::Vector<N, double>& normal,
        const std::array<int, N>& indices,
        const std::vector<numerical::Vector<N, float>>& vertices)
{
        static_assert(!(ASCII && BYTE_SWAP));

        const numerical::Vector<N, float> n = [&]
        {
                const numerical::Vector<N, float> v = to_vector<float>(normal.normalized());
                return is_finite(v) ? v : numerical::Vector<N, float>(0);
        }();

        if constexpr (ASCII)
        {
                write_ascii_facet(file, n, indices, vertices);
        }
        else
        {
                write_binary_facet<BYTE_SWAP>(file, n, indices, vertices);
        }
}

template <bool ASCII, bool BYTE_SWAP, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh, const std::vector<numerical::Vector<N, float>>& vertices)
{
        static_assert(!(ASCII && BYTE_SWAP));

        for (const typename Mesh<N>::Facet& f : mesh.facets)
        {
                if (!f.has_normal)
                {
                        const numerical::Vector<N, double> normal =
                                numerical::orthogonal_complement<double>(vertices, f.vertices);
                        write_facet<ASCII, BYTE_SWAP>(file, normal, f.vertices, vertices);
                }
                else if constexpr (N != 3)
                {
                        const numerical::Vector<N, double> normal =
                                numerical::orthogonal_complement<double>(vertices, f.vertices);
                        write_facet<ASCII, BYTE_SWAP>(file, normal, f.vertices, vertices);
                }
                else
                {
                        std::array<int, 3> v = f.vertices;

                        const numerical::Vector<3, double> v0 = to_vector<double>(mesh.vertices[v[0]]);
                        const numerical::Vector<3, double> v1 = to_vector<double>(mesh.vertices[v[1]]);
                        const numerical::Vector<3, double> v2 = to_vector<double>(mesh.vertices[v[2]]);

                        numerical::Vector<3, double> normal = cross(v1 - v0, v2 - v0);

                        if (dot(to_vector<double>(mesh.normals[f.normals[0]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[1]]), normal) < 0
                            && dot(to_vector<double>(mesh.normals[f.normals[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                normal = -normal;
                        }

                        write_facet<ASCII, BYTE_SWAP>(file, normal, v, vertices);
                }
        }
}

template <bool ASCII, bool BYTE_SWAP, std::size_t N>
void write_facets(std::ostream& file, const Mesh<N>& mesh)
{
        static_assert(!(ASCII && BYTE_SWAP));

        if constexpr (NORMALIZE_VERTEX_COORDINATES)
        {
                std::optional<mesh::BoundingBox<N>> box = mesh::bounding_box_by_facets(mesh);
                if (!box)
                {
                        error("Facet coordinates are not found");
                }
                write_facets<ASCII, BYTE_SWAP>(file, mesh, normalize_vertices(mesh, *box));
        }
        else
        {
                write_facets<ASCII, BYTE_SWAP>(file, mesh, mesh.vertices);
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

        const std::vector<int> facet_indices = unique_facet_indices(mesh);

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

template <bool ASCII, bool BYTE_SWAP, std::size_t N>
void write(std::ostream& file, const Mesh<N>& mesh, const std::string_view comment)
{
        if constexpr (ASCII)
        {
                static_assert(!BYTE_SWAP);
                const std::string solid_name = comment_to_solid_name(comment);
                write_begin_ascii(file, solid_name);
                write_facets<ASCII, BYTE_SWAP>(file, mesh);
                write_end_ascii(file, solid_name);
        }
        else
        {
                write_begin_binary<BYTE_SWAP>(file, mesh.facets.size());
                write_facets<ASCII, BYTE_SWAP>(file, mesh);
                write_end_binary(file);
        }
}
}

template <std::size_t N, typename Path>
std::filesystem::path save_to_stl_file(
        const Mesh<N>& mesh,
        const Path& file_name,
        const std::string_view comment,
        const bool ascii_format,
        const bool byte_swap)
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

        if (ascii_format)
        {
                write<true, false>(file, mesh, comment);
        }
        else
        {
                if (byte_swap)
                {
                        write<false, true>(file, mesh, comment);
                }
                else
                {
                        write<false, false>(file, mesh, comment);
                }
        }

        if (!file)
        {
                error("Error writing to file " + generic_utf8_filename(full_name));
        }

        LOG(stl_type_name(N) + " saved, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return full_name;
}

#define TEMPLATE(N)                                      \
        template std::filesystem::path save_to_stl_file( \
                const Mesh<(N)>&, const std::filesystem::path&, std::string_view, bool, bool);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
