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

#include "load_stl.h"

#include "data_read.h"

#include "../position.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/utility/file/read.h>
#include <src/utility/string/ascii.h>

#include <bit>
#include <cstring>
#include <unordered_map>

namespace mesh::file
{
namespace
{
static_assert(std::endian::native == std::endian::little, "Binary STL numbers must be little-endian");

constexpr std::uintmax_t BINARY_HEADER_SIZE = 80 * sizeof(uint8_t);
constexpr std::uintmax_t BINARY_NUMBER_OF_TRIANGLES_SIZE = sizeof(uint32_t);
constexpr std::uintmax_t BINARY_BEGIN_SIZE = BINARY_HEADER_SIZE + BINARY_NUMBER_OF_TRIANGLES_SIZE;
template <size_t N>
constexpr std::uintmax_t BINARY_NORMAL_SIZE = N * sizeof(float);
template <size_t N>
constexpr std::uintmax_t BINARY_FACETS_SIZE = N* N * sizeof(float);

template <size_t N>
bool is_binary(const std::string& data)
{
        if (data.size() <= BINARY_BEGIN_SIZE)
        {
                return false;
        }

        uint32_t number_of_triangles;
        std::memcpy(&number_of_triangles, &data[BINARY_HEADER_SIZE], sizeof(number_of_triangles));

        const std::uintmax_t required_binary_size =
                BINARY_BEGIN_SIZE + number_of_triangles * (BINARY_NORMAL_SIZE<N> + BINARY_FACETS_SIZE<N>);

        if (data.size() < required_binary_size)
        {
                return false;
        }

        constexpr std::uintmax_t END_SIZE = sizeof(uint16_t);

        if (data.size() > required_binary_size + END_SIZE)
        {
                return false;
        }

        for (char c : data)
        {
                if (!(ascii::is_print(c) || ascii::is_space(c)))
                {
                        return true;
                }
        }

        return false;
}

template <typename Data, typename Word>
void read_keyword(const Data& data, long long data_size, const Word& word, long long* i)
{
        if (*i + static_cast<long long>(word.size()) > data_size)
        {
                error("Keyword " + word + " not found in STL file when expected");
        }
        long long d = *i;
        auto w = word.cbegin();
        while (w != word.cend() && data[d] == *w)
        {
                ++d;
                ++w;
        }
        if (w == word.cend())
        {
                *i = d;
                return;
        }
        error("Keyword " + word + " not found in STL file when expected");
}

template <size_t N>
void read_ascii_stl(
        const std::string& file_data,
        ProgressRatio* progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        const std::string SOLID = "solid";
        const std::string FACET_NORMAL = "facet normal";
        const std::string OUTER_LOOP = "outer loop";
        const std::string VERTEX = "vertex";
        const std::string END_LOOP = "endloop";
        const std::string END_FACET = "endfacet";
        const std::string END_SOLID = "endsolid";

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

template <size_t N>
void read_binary_stl(
        const std::string& file_data,
        ProgressRatio* progress,
        const std::function<void(const std::array<Vector<N, float>, N>&)>& yield_facet)
{
        ASSERT(file_data.size() > BINARY_BEGIN_SIZE);

        uint32_t facet_count;
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

template <size_t N>
std::unique_ptr<Mesh<N>> read_stl(const std::filesystem::path& file_name, ProgressRatio* progress)
{
        std::unordered_map<Vector<N, float>, unsigned> unique_vertices;
        Mesh<N> mesh;

        const auto yield_facet = [&](const std::array<Vector<N, float>, N>& facet_vertices)
        {
                typename Mesh<N>::Facet& facet = mesh.facets.emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        unsigned index;

                        auto iter = unique_vertices.find(facet_vertices[i]);
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

        set_center_and_length(&mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> load_from_stl_file(const std::filesystem::path& file_name, ProgressRatio* progress)
{
        TimePoint start_time = time();

        std::unique_ptr<Mesh<N>> mesh = read_stl<N>(file_name, progress);

        LOG("STL loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_stl_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<4>> load_from_stl_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<5>> load_from_stl_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<6>> load_from_stl_file(const std::filesystem::path& file_name, ProgressRatio* progress);
}
