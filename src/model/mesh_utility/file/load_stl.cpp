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

#include <unordered_map>

namespace mesh::file
{
namespace
{
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
std::unique_ptr<Mesh<N>> read_stl(const std::string& file_name, ProgressRatio* progress)
{
        std::unordered_map<Vector<N, float>, unsigned> unique_vertices;
        Mesh<N> mesh;

        const auto yield_facet = [&](const std::array<Vector<N, float>, N>& facet_vertices) {
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

        read_ascii_stl<N>(file_data, progress, yield_facet);

        set_center_and_length(&mesh);

        return std::make_unique<Mesh<N>>(std::move(mesh));
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        std::unique_ptr<Mesh<N>> mesh = read_stl<N>(file_name, progress);

        LOG("STL loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<4>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<5>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<6>> load_from_stl_file(const std::string& file_name, ProgressRatio* progress);
}
