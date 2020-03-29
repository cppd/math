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
std::unique_ptr<Mesh<N>> read_stl(const std::string& file_name, ProgressRatio* progress)
{
        const std::string SOLID = "solid";
        const std::string FACET_NORMAL = "facet normal";
        const std::string OUTER_LOOP = "outer loop";
        const std::string VERTEX = "vertex";
        const std::string END_LOOP = "endloop";
        const std::string END_FACET = "endfacet";
        const std::string END_SOLID = "endsolid";

        progress->set_undefined();

        std::string s;
        read_text_file(file_name, &s);

        const char* const data = s.c_str();
        const long long size = s.size();
        const double size_reciprocal = 1.0 / size;

        long long i = 0;

        read(data, size, ascii::is_space, &i);
        read_keyword(data, size, SOLID, &i);
        read(data, size, ascii::is_not_new_line, &i);
        ++i;

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

                if (i >= size)
                {
                        error("Normal coordinates not found in STL file when expected");
                }
                Vector<N, float> normal;
                i = read_float(&data[i], &normal) - data;

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
                        Vector<N, float> vertex;
                        i = read_float(&data[i], &vertex) - data;
                }
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

        Mesh<N> mesh;

        try
        {
                set_center_and_length(&mesh);
        }
        catch (...)
        {
                error("STL loading is not implemented");
        }

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
