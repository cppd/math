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

#pragma once
#ifndef OBJ_H
#define OBJ_H

#include <SFML/Graphics/Image.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

struct IObj
{
        struct vertex
        {
                int v;
                int vt; // -1 если нет текстурных координат
                int vn; // -1 если нет нормали
        };
        struct face3
        {
                vertex vertices[3];
                int material; // -1 если нет материала
                bool has_vt, has_vn;
        };
        struct material
        {
                std::string name;
                glm::vec3 Ka{0}, Kd{0}, Ks{0};
                float Ns{1};
                int map_Ka{-1}, map_Kd{-1}, map_Ks{-1}; // -1 если нет текстуры
        };

        virtual ~IObj() = default;

        virtual const std::vector<glm::vec3>& get_vertices() const = 0;
        virtual const std::vector<glm::vec2>& get_texcoords() const = 0;
        virtual const std::vector<glm::vec3>& get_normals() const = 0;
        virtual const std::vector<face3>& get_faces() const = 0;
        virtual const std::vector<material>& get_materials() const = 0;
        virtual const std::vector<sf::Image>& get_images() const = 0;
        virtual glm::vec3 get_center() const = 0;
        virtual float get_length() const = 0;

protected:
        static void find_center_and_length(const std::vector<glm::vec3>& vertices, const std::vector<face3> faces,
                                           glm::vec3* center, float* length);
};

#endif
