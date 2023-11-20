/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "load_stl.h"

#include "data_read.h"
#include "mesh_facet.h"

#include "stl/swap.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/file/read.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/ascii.h>
#include <src/settings/instantiation.h>

#include <cstring>
#include <filesystem>
#include <unordered_map>

namespace ns::model::mesh::file
{
namespace
{
using BinaryNumberOfTriangles = std::uint32_t;

constexpr std::string_view SOLID = "solid";
constexpr std::string_view FACET_NORMAL = "facet normal";
constexpr std::string_view OUTER_LOOP = "outer loop";
constexpr std::string_view VERTEX = "vertex";
constexpr std::string_view END_LOOP = "endloop";
constexpr std::string_view END_FACET = "endfacet";
constexpr std::string_view END_SOLID = "endsolid";

constexpr std::uintmax_t BINARY_NUMBER_OF_TRIANGLES_OFFSET = 80 * sizeof(std::uint8_t);
constexpr std::uintmax_t BINARY_DATA_OFFSET = BINARY_NUMBER_OF_TRIANGLES_OFFSET + sizeof(BinaryNumberOfTriangles);
template <std::size_t N>
constexpr std::uintmax_t BINARY_NORMAL_SIZE = N * sizeof(float);
template <std::size_t N>
constexpr std::uintmax_t BINARY_FACET_SIZE = N * N * sizeof(float);

template <bool BYTE_SWAP>
std::uint32_t binary_number_of_triangles(const std::vector<char>& data)
{
        ASSERT(data.size() >= BINARY_NUMBER_OF_TRIANGLES_OFFSET + sizeof(BinaryNumberOfTriangles));

        BinaryNumberOfTriangles v;
        std::memcpy(&v, &data[BINARY_NUMBER_OF_TRIANGLES_OFFSET], sizeof(v));
        if constexpr (!BYTE_SWAP)
        {
                return v;
        }
        else
        {
                return stl::byte_swap(v);
        }
}

template <std::size_t N, bool BYTE_SWAP>
bool is_binary(const std::vector<char>& data)
{
        if (data.size() <= BINARY_DATA_OFFSET)
        {
                return false;
        }

        const std::uint32_t number_of_triangles = binary_number_of_triangles<BYTE_SWAP>(data);

        const std::uintmax_t required_binary_size =
                BINARY_DATA_OFFSET + number_of_triangles * (BINARY_NORMAL_SIZE<N> + BINARY_FACET_SIZE<N>);

        if (data.size() < required_binary_size)
        {
                return false;
        }

        constexpr std::uintmax_t END_SIZE = sizeof(std::uint16_t);

        if (data.size() > required_binary_size + END_SIZE)
        {
                return false;
        }

        return std::any_of(
                data.cbegin(), data.cend(),
                [](const char c)
                {
                        return !ascii::is_print(c) && !ascii::is_space(c);
                });
}

const char* read_keyword(const char* const first, const char* const last, const std::string_view keyword)
{
        if (first + keyword.size() > last)
        {
                error("Keyword \"" + std::string(keyword) + "\" not found in STL file when expected");
        }

        const char* v = first;
        const char* w = keyword.data();
        const char* const w_end = keyword.data() + keyword.size();

        while (w != w_end && *v == *w)
        {
                ++v;
                ++w;
        }

        if (w == w_end)
        {
                if (v < last && !ascii::is_space(*v))
                {
                        error("Keyword " + std::string(keyword) + " not found in STL file when expected");
                }
                return v;
        }

        error("Keyword " + std::string(keyword) + " not found in STL file when expected");
}

template <std::size_t N>
[[nodiscard]] bool read_normal(const char* const last, const char** const iter)
{
        *iter = read(*iter, last, ascii::is_space);
        try
        {
                *iter = read_keyword(*iter, last, FACET_NORMAL);
        }
        catch (...)
        {
                *iter = read_keyword(*iter, last, END_SOLID);
                *iter = read(*iter, last, ascii::is_not_new_line);
                *iter = read(*iter, last, ascii::is_space);
                if (*iter < last)
                {
                        error("Nonspace found after solid end in STL file");
                }
                return false;
        }
        if (*iter >= last)
        {
                error("Normal coordinates not found in STL file when expected");
        }
        Vector<N, float> n;
        *iter = read(*iter, &n);
        return true;
}

template <std::size_t N>
void read_facet(
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet,
        const char* const last,
        const char** const iter)
{
        std::array<Vector<N, float>, N> facet_vertices;
        for (std::size_t v = 0; v < N; ++v)
        {
                *iter = read(*iter, last, ascii::is_space);
                *iter = read_keyword(*iter, last, VERTEX);
                if (*iter >= last)
                {
                        error("Vertex coordinates not found in STL file when expected");
                }
                *iter = read(*iter, &facet_vertices[v]);
        }
        yield_facet(facet_vertices);
}

template <std::size_t N>
void read_ascii_stl(
        const std::vector<char>& data,
        progress::Ratio* const progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        const double size_reciprocal = 1.0 / data.size();

        const char* const first = data.data();

        ASSERT(!data.empty() && !data.back());
        const char* const last = data.data() + data.size() - 1;

        const char* iter = first;

        iter = read(iter, last, ascii::is_space);
        iter = read_keyword(iter, last, SOLID);
        iter = read(iter, last, ascii::is_not_new_line);

        unsigned long long facet_count = 0;

        while (read_normal<N>(last, &iter))
        {
                iter = read(iter, last, ascii::is_space);
                iter = read_keyword(iter, last, OUTER_LOOP);

                read_facet(yield_facet, last, &iter);

                iter = read(iter, last, ascii::is_space);
                iter = read_keyword(iter, last, END_LOOP);

                iter = read(iter, last, ascii::is_space);
                iter = read_keyword(iter, last, END_FACET);

                if (((++facet_count) & 0xfff) == 0xfff)
                {
                        progress->set((iter - first) * size_reciprocal);
                }
        }

        LOG("STL facet count: " + to_string(facet_count));
}

template <std::size_t N, bool BYTE_SWAP>
void read_binary_stl(
        const std::vector<char>& data,
        progress::Ratio* const progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        ASSERT(data.size() >= BINARY_DATA_OFFSET);

        const std::uint32_t facet_count = binary_number_of_triangles<BYTE_SWAP>(data);
        ASSERT(BINARY_DATA_OFFSET + facet_count * (BINARY_NORMAL_SIZE<N> + BINARY_FACET_SIZE<N>) <= data.size());

        const char* read_ptr = &data[BINARY_DATA_OFFSET];
        const double facet_count_reciprocal = 1.0 / facet_count;

        std::array<Vector<N, std::conditional_t<!BYTE_SWAP, float, std::uint32_t>>, N> facet_vertices;
        static_assert(sizeof(facet_vertices) == BINARY_FACET_SIZE<N>);

        read_ptr += BINARY_NORMAL_SIZE<N>;
        for (unsigned facet = 0; facet < facet_count; ++facet)
        {
                if ((facet & 0xfff) == 0xfff)
                {
                        progress->set(facet * facet_count_reciprocal);
                }

                std::memcpy(&facet_vertices, read_ptr, sizeof(facet_vertices));
                if constexpr (!BYTE_SWAP)
                {
                        yield_facet(facet_vertices);
                }
                else
                {
                        yield_facet(stl::byte_swap(facet_vertices));
                }

                read_ptr += BINARY_NORMAL_SIZE<N> + BINARY_FACET_SIZE<N>;
        }

        LOG("STL facet count: " + to_string(facet_count));
}

template <std::size_t N, bool BYTE_SWAP>
std::unique_ptr<Mesh<N>> read_stl(const std::filesystem::path& file_name, progress::Ratio* const progress)
{
        std::unordered_map<Vector<N, float>, unsigned> unique_vertices;
        Mesh<N> mesh;

        const auto yield_facet = [&](const std::array<Vector<N, float>, N>& facet_vertices)
        {
                typename Mesh<N>::Facet& facet = mesh.facets.emplace_back();

                for (std::size_t i = 0; i < N; ++i)
                {
                        facet.vertices[i] = [&]
                        {
                                const auto iter = unique_vertices.find(facet_vertices[i]);
                                if (iter != unique_vertices.cend())
                                {
                                        return iter->second;
                                }
                                const unsigned index = mesh.vertices.size();
                                mesh.vertices.push_back(facet_vertices[i]);
                                unique_vertices.emplace(facet_vertices[i], index);
                                return index;
                        }();

                        facet.normals[i] = -1;
                        facet.texcoords[i] = -1;
                        facet.material = -1;
                        facet.has_texcoord = false;
                        facet.has_normal = false;
                }
        };

        progress->set_undefined();

        std::vector<char> data = read_file(file_name);

        if (is_binary<N, BYTE_SWAP>(std::as_const(data)))
        {
                read_binary_stl<N, BYTE_SWAP>(data, progress, yield_facet);
        }
        else
        {
                data.push_back('\0');
                read_ascii_stl<N>(data, progress, yield_facet);
        }

        check_and_correct_mesh_facets(&mesh);
        set_center_and_length(&mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_stl_file(
        const Path& file_name,
        progress::Ratio* const progress,
        const bool byte_swap)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh =
                byte_swap ? read_stl<N, true>(file_name, progress) : read_stl<N, false>(file_name, progress);

        LOG("STL loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

#define TEMPLATE(N) \
        template std::unique_ptr<Mesh<(N)>> load_from_stl_file(const std::filesystem::path&, progress::Ratio*, bool);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
