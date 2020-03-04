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

#include "../alg/alg.h"
#include "read/data.h"
#include "read/lines.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/time.h>

namespace
{
// Чтение вершин из текстового файла. Одна вершина на строку. Координаты через пробел.
// x0 x1 x2 x3 ...
// x0 x1 x2 x3 ...
template <size_t N>
class FileTxt final : public MeshModel<N>
{
        using typename MeshModel<N>::Facet;
        using typename MeshModel<N>::Point;
        using typename MeshModel<N>::Line;
        using typename MeshModel<N>::Material;
        using typename MeshModel<N>::Image;

        std::vector<Vector<N, float>> m_vertices;
        std::vector<Vector<N, float>> m_normals;
        std::vector<Vector<N - 1, float>> m_texcoords;
        std::vector<Facet> m_facets;
        std::vector<Point> m_points;
        std::vector<Line> m_lines;
        std::vector<Material> m_materials;
        std::vector<Image> m_images;
        Vector<N, float> m_center;
        float m_length;

        void read_points_thread(
                unsigned thread_num,
                unsigned thread_count,
                std::vector<char>* data_ptr,
                const std::vector<long long>& line_begin,
                std::vector<Vector<N, float>>* lines,
                ProgressRatio* progress) const;
        void read_points(const std::string& file_name, ProgressRatio* progress);
        void read_text(const std::string& file_name, ProgressRatio* progress);

        const std::vector<Vector<N, float>>& vertices() const override
        {
                return m_vertices;
        }
        const std::vector<Vector<N, float>>& normals() const override
        {
                return m_normals;
        }
        const std::vector<Vector<N - 1, float>>& texcoords() const override
        {
                return m_texcoords;
        }
        const std::vector<Facet>& facets() const override
        {
                return m_facets;
        }
        const std::vector<Point>& points() const override
        {
                return m_points;
        }
        const std::vector<Line>& lines() const override
        {
                return m_lines;
        }
        const std::vector<Material>& materials() const override
        {
                return m_materials;
        }
        const std::vector<Image>& images() const override
        {
                return m_images;
        }
        Vector<N, float> center() const override
        {
                return m_center;
        }
        float length() const override
        {
                return m_length;
        }

public:
        FileTxt(const std::string& file_name, ProgressRatio* progress);
};

template <size_t N>
void FileTxt<N>::read_points_thread(
        unsigned thread_num,
        unsigned thread_count,
        std::vector<char>* data_ptr,
        const std::vector<long long>& line_begin,
        std::vector<Vector<N, float>>* lines,
        ProgressRatio* progress) const
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
                catch (std::exception& e)
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
void FileTxt<N>::read_points(const std::string& file_name, ProgressRatio* progress)
{
        const int thread_count = hardware_concurrency();

        std::vector<char> file_data;
        std::vector<long long> line_begin;

        read_file_lines(file_name, &file_data, &line_begin);

        m_vertices.resize(line_begin.size());

        ThreadsWithCatch threads(thread_count);
        for (int i = 0; i < thread_count; ++i)
        {
                threads.add([&, i]() {
                        read_points_thread(i, thread_count, &file_data, line_begin, &m_vertices, progress);
                });
        }
        threads.join();
}

template <size_t N>
void FileTxt<N>::read_text(const std::string& file_name, ProgressRatio* progress)
{
        progress->set_undefined();

        read_points(file_name, progress);

        if (m_vertices.empty())
        {
                error("No vertices found in TXT file");
        }

        m_points.resize(m_vertices.size());
        for (unsigned i = 0; i < m_points.size(); ++i)
        {
                m_points[i].vertex = i;
        }

        center_and_length(m_vertices, m_points, &m_center, &m_length);
}

template <size_t N>
FileTxt<N>::FileTxt(const std::string& file_name, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        read_text(file_name, progress);

        LOG("TEXT loaded, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}
}

template <size_t N>
std::unique_ptr<MeshModel<N>> load_from_txt_file(const std::string& file_name, ProgressRatio* progress)
{
        return std::make_unique<FileTxt<N>>(file_name, progress);
}

template std::unique_ptr<MeshModel<3>> load_from_txt_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<MeshModel<4>> load_from_txt_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<MeshModel<5>> load_from_txt_file(const std::string& file_name, ProgressRatio* progress);
template std::unique_ptr<MeshModel<6>> load_from_txt_file(const std::string& file_name, ProgressRatio* progress);
