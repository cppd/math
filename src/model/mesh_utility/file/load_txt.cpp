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

#include "load_txt.h"

#include "data_read.h"
#include "file_lines.h"

#include "../position.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/com/time.h>

namespace mesh::file
{
namespace
{
// Чтение вершин из текстового файла. Одна вершина на строку. Координаты через пробел.
// x0 x1 x2 x3 ...
// x0 x1 x2 x3 ...
template <size_t N>
void read_points_thread(
        unsigned thread_num,
        unsigned thread_count,
        std::vector<char>* data_ptr,
        const std::vector<long long>& line_begin,
        std::vector<Vector<N, float>>* lines,
        ProgressRatio* progress)
{
        const long long line_count = line_begin.size();
        const double line_count_reciprocal = 1.0 / line_begin.size();

        for (long long line_num = thread_num; line_num < line_count; line_num += thread_count)
        {
                if ((line_num & 0xfff) == 0xfff)
                {
                        progress->set(line_num * line_count_reciprocal);
                }

                const char* str = &(*data_ptr)[line_begin[line_num]];

                long long last = (line_num < line_count - 1) ? line_begin[line_num + 1] : data_ptr->size();

                // В конце строки находится символ '\n', сместиться на него и записать вместо него 0
                --last;

                (*data_ptr)[last] = 0;

                try
                {
                        read_float(str, &(*lines)[line_num]);
                }
                catch (const std::exception& e)
                {
                        error("Line " + to_string(line_num) + ": " + str + "\n" + e.what());
                }
                catch (...)
                {
                        error("Line " + to_string(line_num) + ": " + str + "\n" + "Unknown error");
                }
        }
}

template <size_t N>
void read_points(
        std::vector<Vector<N, float>>* vertices,
        const std::filesystem::path& file_name,
        ProgressRatio* progress)
{
        const int thread_count = hardware_concurrency();

        std::vector<char> file_data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &file_data, &line_begin);

        vertices->resize(line_begin.size());

        ThreadsWithCatch threads(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
                threads.add(
                        [&, i]()
                        {
                                read_points_thread(i, thread_count, &file_data, line_begin, vertices, progress);
                        });
        }
        threads.join();
}

template <size_t N>
std::unique_ptr<Mesh<N>> read_text(const std::filesystem::path& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        std::unique_ptr<Mesh<N>> mesh = std::make_unique<Mesh<N>>();

        read_points(&mesh->vertices, file_name, progress);

        if (mesh->vertices.empty())
        {
                error("No vertices found in TXT file");
        }

        mesh->points.resize(mesh->vertices.size());
        for (unsigned i = 0; i < mesh->points.size(); ++i)
        {
                mesh->points[i].vertex = i;
        }

        set_center_and_length(mesh.get());

        return mesh;
}
}

template <size_t N>
std::unique_ptr<Mesh<N>> load_from_txt_file(const std::filesystem::path& file_name, ProgressRatio* progress)
{
        TimePoint start_time = time();

        std::unique_ptr<Mesh<N>> mesh = read_text<N>(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(duration_from(start_time), 5) + " s");

        return mesh;
}

template std::unique_ptr<Mesh<3>> load_from_txt_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<4>> load_from_txt_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<5>> load_from_txt_file(const std::filesystem::path& file_name, ProgressRatio* progress);
template std::unique_ptr<Mesh<6>> load_from_txt_file(const std::filesystem::path& file_name, ProgressRatio* progress);
}
