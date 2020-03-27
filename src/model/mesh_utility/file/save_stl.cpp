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
#include "../file.h"
#include "../unique.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/numerical/orthogonal.h>
#include <src/utility/file/file.h>
#include <src/utility/file/sys.h>

namespace mesh::file
{
namespace
{
// clang-format off
constexpr const char* SOLID        = "solid s";
constexpr const char* FACET_NORMAL = "facet normal";
constexpr const char* OUTER_LOOP   = "  outer loop";
constexpr const char* VERTEX       = "    vertex";
constexpr const char* END_LOOP     = "  endloop";
constexpr const char* END_FACET    = "endfacet";
constexpr const char* END_SOLID    = "endsolid";
// clang-format on

void write_begin(const CFile& file)
{
        fprintf(file, "%s\n", SOLID);
}

void write_end(const CFile& file)
{
        fprintf(file, "%s\n", END_SOLID);
}

template <size_t N, typename T>
void write_vector(const CFile& file, const Vector<N, T>& vector)
{
        static_assert(limits<float>::max_digits10 <= 9);
        static_assert(limits<double>::max_digits10 <= 17);
        static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);

        constexpr const char* format = (std::is_same_v<T, float>) ? " %12.9f" : " %20.17f";

        for (unsigned i = 0; i < N; ++i)
        {
                fprintf(file, format, vector[i]);
        }
}

template <size_t N>
void write_facet(
        const CFile& file,
        const Vector<N, double>& normal,
        const std::array<int, N>& indices,
        const std::vector<Vector<N, float>>& vertices)
{
        Vector<N, float> n = to_vector<float>(normal.normalized());
        if (!is_finite(n))
        {
                n = Vector<N, float>(0);
        }

        fprintf(file, "%s", FACET_NORMAL);
        write_vector(file, n);
        fprintf(file, "\n");

        fprintf(file, "%s\n", OUTER_LOOP);
        for (unsigned i = 0; i < N; ++i)
        {
                fprintf(file, "%s", VERTEX);
                write_vector(file, vertices[indices[i]]);
                fprintf(file, "\n");
        }
        fprintf(file, "%s\n", END_LOOP);
        fprintf(file, "%s\n", END_FACET);
}

// Преообразование координат вершин с приведением к интервалу [-1, 1] с сохранением пропорций
template <size_t N>
std::vector<Vector<N, float>> normalize_vertices(const Mesh<N>& mesh)
{
        std::vector<int> facet_indices = unique_facet_indices(mesh);

        if (facet_indices.empty())
        {
                error("Facet unique indices are not found");
        }
        if (!facet_indices.empty() && facet_indices.size() < N)
        {
                error("Facet unique indices count " + to_string(facet_indices.size()) + " is less than " +
                      to_string(N));
        }

        std::optional<mesh::BoundingBox<N>> box = mesh::bounding_box_by_facets(mesh);
        if (!box)
        {
                error("Facet coordinates are not found");
        }

        Vector<N, float> extent = box->max - box->min;

        float max_extent = extent.norm_infinity();

        if (max_extent == 0)
        {
                error("Mesh vertices are equal to each other");
        }

        float scale_factor = 2 / max_extent;
        Vector<N, float> center = box->min + 0.5f * extent;

        std::vector<Vector<N, float>> normalized_vertices;
        normalized_vertices.reserve(mesh.vertices.size());

        for (const Vector<N, float>& v : mesh.vertices)
        {
                Vector<N, float> vertex = (v - center) * scale_factor;

                normalized_vertices.push_back(vertex);
        }

        return normalized_vertices;
}

template <size_t N>
void write_facets(const CFile& file, const Mesh<N>& mesh)
{
        const std::vector<Vector<N, float>> vertices = normalize_vertices(mesh);

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
                        write_facet(file, normal, f.vertices, vertices);
                }
                else if constexpr (N != 3)
                {
                        Vector<N, double> normal = ortho_nn<N, float, double>(vertices, f.vertices);
                        write_facet(file, normal, f.vertices, vertices);
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
                        if (dot(to_vector<double>(mesh.normals[f.normals[0]]), normal) < 0 &&
                            dot(to_vector<double>(mesh.normals[f.normals[1]]), normal) < 0 &&
                            dot(to_vector<double>(mesh.normals[f.normals[2]]), normal) < 0)
                        {
                                std::swap(v[1], v[2]);
                                normal = -normal;
                        }

                        write_facet(file, normal, v, vertices);
                }
        }
}

std::string stl_type_name(size_t N)
{
        return "STL-" + to_string(N);
}

template <size_t N>
std::string file_name_with_extension(const std::string& file_name)
{
        std::string e = file_extension(file_name);

        if (!e.empty())
        {
                if (!is_stl_file_extension(N, e))
                {
                        error("Wrong " + stl_type_name(N) + " file name extension: " + e);
                }

                return file_name;
        }

        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + stl_file_extension(N);
}
}

template <size_t N>
std::string save_to_stl_file(const Mesh<N>& mesh, const std::string& file_name)
{
        static_assert(N >= 3);

        if (mesh.facets.empty())
        {
                error("Mesh has no facets");
        }

        std::string full_name = file_name_with_extension<N>(file_name);

        CFile file(full_name, "w");

        double start_time = time_in_seconds();

        write_begin(file);
        write_facets(file, mesh);
        write_end(file);

        LOG(stl_type_name(N) + " saved, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");

        return full_name;
}

template std::string save_to_stl_file(const Mesh<3>&, const std::string&);
template std::string save_to_stl_file(const Mesh<4>&, const std::string&);
template std::string save_to_stl_file(const Mesh<5>&, const std::string&);
template std::string save_to_stl_file(const Mesh<6>&, const std::string&);
}
