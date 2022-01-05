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

#include "load_stl.h"

#include "data_read.h"
#include "mesh_facet.h"

#include "../position.h"

#include <src/com/chrono.h>
#include <src/com/file/read.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/string/ascii.h>

#include <bit>
#include <cstring>
#include <filesystem>
#include <unordered_map>

namespace ns::mesh::file
{
namespace
{
static_assert(std::endian::native == std::endian::little, "Binary STL numbers must be little-endian");

constexpr std::uintmax_t BINARY_HEADER_SIZE = 80 * sizeof(std::uint8_t);
constexpr std::uintmax_t BINARY_NUMBER_OF_TRIANGLES_SIZE = sizeof(std::uint32_t);
constexpr std::uintmax_t BINARY_BEGIN_SIZE = BINARY_HEADER_SIZE + BINARY_NUMBER_OF_TRIANGLES_SIZE;
template <std::size_t N>
constexpr std::uintmax_t BINARY_NORMAL_SIZE = N * sizeof(float);
template <std::size_t N>
constexpr std::uintmax_t BINARY_FACETS_SIZE = N* N * sizeof(float);

template <std::size_t N>
bool is_binary(const std::string& data)
{
        if (data.size() <= BINARY_BEGIN_SIZE)
        {
                return false;
        }

        std::uint32_t number_of_triangles;
        std::memcpy(&number_of_triangles, &data[BINARY_HEADER_SIZE], sizeof(number_of_triangles));

        const std::uintmax_t required_binary_size =
                BINARY_BEGIN_SIZE + number_of_triangles * (BINARY_NORMAL_SIZE<N> + BINARY_FACETS_SIZE<N>);

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
                [](char c)
                {
                        return !ascii::is_print(c) && !ascii::is_space(c);
                });
}

template <typename Data>
void read_keyword(const Data& data, const long long data_size, const std::string_view& word, long long* const i)
{
        if (*i + static_cast<long long>(word.size()) > data_size)
        {
                error("Keyword " + std::string(word) + " not found in STL file when expected");
        }
        long long d = *i;
        const char* w = word.data();
        const char* const end = word.data() + word.size();
        while (w != end && data[d] == *w)
        {
                ++d;
                ++w;
        }
        if (w == end)
        {
                *i = d;
                return;
        }
        error("Keyword " + std::string(word) + " not found in STL file when expected");
}

template <std::size_t N>
void read_ascii_stl(
        const std::string& file_data,
        ProgressRatio* const progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        static constexpr std::string_view SOLID = "solid";
        static constexpr std::string_view FACET_NORMAL = "facet normal";
        static constexpr std::string_view OUTER_LOOP = "outer loop";
        static constexpr std::string_view VERTEX = "vertex";
        static constexpr std::string_view END_LOOP = "endloop";
        static constexpr std::string_view END_FACET = "endfacet";
        static constexpr std::string_view END_SOLID = "endsolid";

        const char* const data = file_data.c_str();
        const long long size = file_data.size();
        const double size_reciprocal = 1.0 / size;

        long long i = 0;

        read(data, size, ascii::is_space, &i);
        read_keyword(data, size, SOLID, &i);
        read(data, size, ascii::is_not_new_line, &i);
        ++i;

        std::array<Vector<N, float>, N> facet_vertices;
        unsigned facet_count = 0;
        while (true)
        {
                read(data, size, ascii::is_space, &i);
                try
                {
                        read_keyword(data, size, FACET_NORMAL, &i);
                }
                catch (...)
                {
                        read_keyword(data, size, END_SOLID, &i);
                        break;
                }

                // skip normal
                read(data, size, ascii::is_not_new_line, &i);

                read(data, size, ascii::is_space, &i);
                read_keyword(data, size, OUTER_LOOP, &i);

                for (unsigned v = 0; v < N; ++v)
                {
                        read(data, size, ascii::is_space, &i);
                        read_keyword(data, size, VERTEX, &i);

                        if (i >= size)
                        {
                                error("Vertex coordinates not found in STL file when expected");
                        }
                        i = read_float(&data[i], &facet_vertices[v]) - data;
                }
                yield_facet(facet_vertices);

                read(data, size, ascii::is_space, &i);
                read_keyword(data, size, END_LOOP, &i);

                read(data, size, ascii::is_space, &i);
                read_keyword(data, size, END_FACET, &i);

                if (((++facet_count) & 0xfff) == 0xfff)
                {
                        progress->set(i * size_reciprocal);
                }
        }

        LOG("STL facet count: " + to_string(facet_count));
}

template <std::size_t N>
void read_binary_stl(
        const std::string& file_data,
        ProgressRatio* const progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        ASSERT(file_data.size() > BINARY_BEGIN_SIZE);

        std::uint32_t facet_count;
        std::memcpy(&facet_count, &file_data[BINARY_HEADER_SIZE], sizeof(facet_count));

        ASSERT(BINARY_BEGIN_SIZE + facet_count * (BINARY_NORMAL_SIZE<N> + BINARY_FACETS_SIZE<N>) <= file_data.size());

        const char* read_ptr = &file_data[BINARY_BEGIN_SIZE];
        const double facet_count_reciprocal = 1.0 / facet_count;

        std::array<Vector<N, float>, N> facet_vertices;
        static_assert(sizeof(facet_vertices) == BINARY_FACETS_SIZE<N>);

        read_ptr += BINARY_NORMAL_SIZE<N>;
        for (unsigned facet = 0; facet < facet_count; ++facet)
        {
                if ((facet & 0xfff) == 0xfff)
                {
                        progress->set(facet * facet_count_reciprocal);
                }

                std::memcpy(&facet_vertices, read_ptr, sizeof(facet_vertices));
                yield_facet(facet_vertices);

                read_ptr += BINARY_NORMAL_SIZE<N> + BINARY_FACETS_SIZE<N>;
        }

        LOG("STL facet count: " + to_string(facet_count));
}

template <std::size_t N>
std::unique_ptr<Mesh<N>> read_stl(const std::filesystem::path& file_name, ProgressRatio* const progress)
{
        std::unordered_map<Vector<N, float>, unsigned> unique_vertices;
        Mesh<N> mesh;

        const auto yield_facet = [&](const std::array<Vector<N, float>, N>& facet_vertices)
        {
                typename Mesh<N>::Facet& facet = mesh.facets.emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        unsigned index;

                        const auto iter = unique_vertices.find(facet_vertices[i]);
                        if (iter != unique_vertices.cend())
                        {
                                index = iter->second;
                        }
                        else
                        {
                                index = mesh.vertices.size();
                                mesh.vertices.push_back(facet_vertices[i]);
                                unique_vertices.emplace(facet_vertices[i], index);
                        }

                        facet.vertices[i] = index;
                        facet.normals[i] = -1;
                        facet.texcoords[i] = -1;
                        facet.material = -1;
                        facet.has_texcoord = false;
                        facet.has_normal = false;
                }
        };

        progress->set_undefined();

        std::string file_data;
        read_binary_file(file_name, &file_data);

        if (is_binary<N>(file_data))
        {
                read_binary_stl<N>(file_data, progress, yield_facet);
        }
        else
        {
                read_ascii_stl<N>(file_data, progress, yield_facet);
        }

        check_and_correct_mesh_facets(&mesh);
        set_center_and_length(&mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load_from_stl_file(const Path& file_name, ProgressRatio* const progress)
{
        const Clock::time_point start_time = Clock::now();

        std::unique_ptr<Mesh<N>> mesh = read_stl<N>(file_name, progress);

        LOG("STL loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_stl_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<4>> load_from_stl_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<5>> load_from_stl_file(const std::filesystem::path&, ProgressRatio*);
template std::unique_ptr<Mesh<6>> load_from_stl_file(const std::filesystem::path&, ProgressRatio*);
}
