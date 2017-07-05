/*
Copyright (C) 2017 Topological Manifold

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

#include "com/file.h"
#include "com/print.h"
#include "com/time.h"

#include <glm/geometric.hpp>

constexpr const char* comment_begin = "# ";

constexpr const char* VERTEX_FORMAT = "v %11.8f %11.8f %11.8f\n";
constexpr const char* NORMAL_FORMAT = "vn %11.8f %11.8f %11.8f\n";

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
        std::vector<int> indices = get_unique_face_indices(obj->get_faces());

        if (indices.size() < 3)
        {
                error("face unique indices count < 3 ");
        }

        glm::vec3 min, max;

        find_min_max(obj->get_vertices(), indices, &min, &max);

        glm::dvec3 delta = glm::dvec3(max - min);

        double max_delta = std::max({delta[0], delta[1], delta[2]});

        if (max_delta == 0)
        {
                for (const glm::vec3& v : obj->get_vertices())
                {
                        glm::dvec3 vertex = glm::dvec3(v);
                        fprintf(file, VERTEX_FORMAT, vertex[0], vertex[1], vertex[2]);
                }
        }
        else
        {
                double scale_factor = 2.0 / max_delta;

                glm::dvec3 old_min = glm::dvec3(min);
                glm::dvec3 new_min = 0.5 * delta * scale_factor;

                for (const glm::vec3& v : obj->get_vertices())
                {
                        glm::dvec3 vertex = (glm::dvec3(v) - old_min) * scale_factor - new_min;
                        fprintf(file, VERTEX_FORMAT, vertex[0], vertex[1], vertex[2]);
                }
        }
}

void write_normals(const CFile& file, const IObj* obj)
{
        for (const glm::vec3& vn : obj->get_normals())
        {
                glm::dvec3 normal = glm::dvec3(vn);

                double length = glm::length(normal);

                if (length != 0)
                {
                        normal /= length;
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

        for (const IObj::face3& f : obj->get_faces())
        {
                if (f.has_vn)
                {
                        // В файлах OBJ номера начинаются с 1
                        long v0 = f.vertices[0].v + 1;
                        long vn0 = f.vertices[0].vn + 1;
                        long v1 = f.vertices[1].v + 1;
                        long vn1 = f.vertices[1].vn + 1;
                        long v2 = f.vertices[2].v + 1;
                        long vn2 = f.vertices[2].vn + 1;

                        // Перпендикуляр к грани при обходе вершин против часовой стрелки
                        // и противоположно направлению взгляда
                        glm::vec3 n = glm::cross(obj->get_vertices()[v1 - 1] - obj->get_vertices()[v0 - 1],
                                                 obj->get_vertices()[v2 - 1] - obj->get_vertices()[v0 - 1]);

                        float sign0 = glm::dot(obj->get_normals()[vn0 - 1], n);
                        float sign1 = glm::dot(obj->get_normals()[vn1 - 1], n);
                        float sign2 = glm::dot(obj->get_normals()[vn2 - 1], n);

                        if (sign0 > 0 && sign1 > 0 && sign2 > 0)
                        {
                                // Если перпендикуляр в одном направлении со всеми векторами вершин,
                                // то сохраняется порядок вершин
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, vn0, v1, vn1, v2, vn2);
                        }
                        else if (sign0 < 0 && sign1 < 0 && sign2 < 0)
                        {
                                // Если перпендикуляр в противоположном направлении со всеми векторами вершин,
                                // то меняется порядок вершин
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, vn0, v2, vn2, v1, vn1);
                        }
                        else
                        {
                                // Не удалось определить, поэтому вершины записываются в произвольном порядке
                                fprintf(file, "f %ld//%ld %ld//%ld %ld//%ld\n", v0, vn0, v1, vn1, v2, vn2);
                        }
                }
                else
                {
                        // Нет нормалей у вершин, поэтому вершины грани записываются в произвольном порядке
                        long v0 = f.vertices[0].v + 1;
                        long v1 = f.vertices[1].v + 1;
                        long v2 = f.vertices[2].v + 1;
                        fprintf(file, "f %ld %ld %ld\n", v0, v1, v2);
                }
        }
}
}

void save_obj_geometry_to_file(const IObj* obj, const std::string& file_name, const std::string& comment)
{
        if (obj->get_faces().size() == 0)
        {
                error("Object doesn't have faces");
        }

        double start_time = get_time_seconds();

        CFile file(file_name, "w");

        write_comment(file, comment);

        write_vertices(file, obj);

        write_normals(file, obj);

        write_faces(file, obj);

        LOG("OBJ saved, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}
