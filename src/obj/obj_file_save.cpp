/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "obj_file_save.h"

#include "obj_alg.h"

#include "com/file/file.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"

constexpr const char comment_begin[] = "# ";

constexpr const char VERTEX_FORMAT[] = "v %11.8f %11.8f %11.8f\n";
constexpr const char NORMAL_FORMAT[] = "vn %11.8f %11.8f %11.8f\n";

namespace
{
void write_comment(const CFile& file, const std::string& comment)
{
        if (comment.size() > 0)
        {
                std::string comment_edited;

                comment_edited += comment_begin;
                for (char c : comment)
                {
                        if (c != '\n')
                        {
                                comment_edited += c;
                        }
                        else
                        {
                                comment_edited += '\n';
                                comment_edited += comment_begin;
                        }
                }
                comment_edited += '\n';

                fprintf(file, "%s", comment_edited.c_str());
        }
}

// Запись вершин с приведением координат вершин граней к интервалу [-1, 1] с сохранением пропорций
void write_vertices(const CFile& file, const IObj* obj)
{
        std::vector<int> indices = unique_face_indices(obj->faces());

        if (indices.size() < 3)
        {
                error("face unique indices count < 3 ");
        }

        vec3f min, max;

        min_max_coordinates(obj->vertices(), indices, &min, &max);

        vec3 delta = to_vector<double>(max - min);

        double max_delta = max_element(delta);

        if (max_delta == 0)
        {
                for (const vec3f& v : obj->vertices())
                {
                        fprintf(file, VERTEX_FORMAT, v[0], v[1], v[2]);
                }
        }
        else
        {
                double scale_factor = 2.0 / max_delta;

                vec3 center = to_vector<double>(min) + 0.5 * delta;

                for (const vec3f& v : obj->vertices())
                {
                        vec3 vertex = (to_vector<double>(v) - center) * scale_factor;
                        fprintf(file, VERTEX_FORMAT, vertex[0], vertex[1], vertex[2]);
                }
        }
}

void write_normals(const CFile& file, const IObj* obj)
{
        for (const vec3f& vn : obj->normals())
        {
                vec3 normal = to_vector<double>(vn);
                double len = length(normal);
                if (len != 0)
                {
                        normal /= len;
                }

                fprintf(file, NORMAL_FORMAT, normal[0], normal[1], normal[2]);
        }
}

void write_faces(const CFile& file, const IObj* obj)
{
        // Вершины граней надо записывать в OBJ таким образом, чтобы при обходе против часовой стрелки
        // перпендикуляр к грани был направлен противоположно направлению взгляда.
        // В данной модели нет перпендикуляров у граней, есть только у вершин, поэтому нужно попытаться
        // определить правильное направление по векторам вершин, если у вершин они заданы.

        for (const IObj::Face& f : obj->faces())
        {
                if (f.has_normal)
                {
                        // В файлах OBJ номера начинаются с 1

                        long v0 = f.vertices[0] + 1;
                        long v1 = f.vertices[1] + 1;
                        long v2 = f.vertices[2] + 1;

                        long n0 = f.normals[0] + 1;
                        long n1 = f.normals[1] + 1;
                        long n2 = f.normals[2] + 1;

                        // Перпендикуляр к грани при обходе вершин против часовой стрелки
                        // и противоположно направлению взгляда
                        vec3f normal = cross(obj->vertices()[v1 - 1] - obj->vertices()[v0 - 1],
                                             obj->vertices()[v2 - 1] - obj->vertices()[v0 - 1]);

                        float sign0 = dot(obj->normals()[n0 - 1], normal);
                        float sign1 = dot(obj->normals()[n1 - 1], normal);
                        float sign2 = dot(obj->normals()[n2 - 1], normal);

                        if (sign0 > 0 && sign1 > 0 && sign2 > 0)
                        {
                                // Если перпендикуляр в одном направлении со всеми векторами вершин,
                                // то сохраняется порядок вершин
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, n0, v1, n1, v2, n2);
                        }
                        else if (sign0 < 0 && sign1 < 0 && sign2 < 0)
                        {
                                // Если перпендикуляр в противоположном направлении со всеми векторами вершин,
                                // то меняется порядок вершин
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, n0, v2, n2, v1, n1);
                        }
                        else
                        {
                                // Не удалось определить, поэтому вершины записываются в произвольном порядке
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, n0, v1, n1, v2, n2);
                        }
                }
                else
                {
                        // Нет нормалей у вершин, поэтому вершины грани записываются в произвольном порядке

                        long v0 = f.vertices[0] + 1;
                        long v1 = f.vertices[1] + 1;
                        long v2 = f.vertices[2] + 1;

                        fprintf(file, "f %ld %ld %ld\n", v0, v1, v2);
                }
        }
}
}

void save_obj_geometry_to_file(const IObj* obj, const std::string& file_name, const std::string& comment)
{
        if (obj->faces().size() == 0)
        {
                error("Object doesn't have faces");
        }

        double start_time = time_in_seconds();

        CFile file(file_name, "w");

        write_comment(file, comment);

        write_vertices(file, obj);

        write_normals(file, obj);

        write_faces(file, obj);

        LOG("OBJ saved, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}
