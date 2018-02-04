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

#pragma once

#include "com/vec.h"

#include <string>
#include <vector>

struct IObj
{
        struct Face
        {
                int vertices[3];
                int normals[3]; // -1 если нет нормали
                int texcoords[3]; // -1 если нет текстурных координат
                int material; // -1 если нет материала
                bool has_texcoord;
                bool has_normal;
        };

        struct Point
        {
                int vertex;
        };

        struct Line
        {
                int vertices[2];
        };

        struct Material
        {
                std::string name;
                vec3f Ka{0};
                vec3f Kd{0};
                vec3f Ks{0};
                float Ns{1};
                int map_Ka{-1}; // -1 если нет текстуры
                int map_Kd{-1}; // -1 если нет текстуры
                int map_Ks{-1}; // -1 если нет текстуры
        };

        struct Image
        {
                int dimensions[2];
                // Цветовое пространство sRGB, последовательность red, green, blue, alpha.
                // Каждый цветовой компонент в интервале [0, 255].
                std::vector<unsigned char> srgba_pixels;
        };

        virtual ~IObj() = default;

        virtual const std::vector<vec3f>& vertices() const = 0;
        virtual const std::vector<vec2f>& texcoords() const = 0;
        virtual const std::vector<vec3f>& normals() const = 0;
        virtual const std::vector<Face>& faces() const = 0;
        virtual const std::vector<Point>& points() const = 0;
        virtual const std::vector<Line>& lines() const = 0;
        virtual const std::vector<Material>& materials() const = 0;
        virtual const std::vector<Image>& images() const = 0;
        virtual vec3f center() const = 0;
        virtual float length() const = 0;
};
