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

#pragma once

#include <src/color/color.h>
#include <src/numerical/vec.h>

#include <array>
#include <string>
#include <vector>

namespace mesh
{
template <size_t N>
struct Mesh final
{
        struct Facet
        {
                std::array<int, N> vertices;
                std::array<int, N> normals; // -1 если нет нормали
                std::array<int, N> texcoords; // -1 если нет текстурных координат
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
                std::array<int, 2> vertices;
        };

        struct Material
        {
                std::string name;
                Color Ka{0};
                Color Kd{0};
                Color Ks{0};
                float Ns{1};
                int map_Ka{-1}; // -1 если нет текстуры
                int map_Kd{-1}; // -1 если нет текстуры
                int map_Ks{-1}; // -1 если нет текстуры
        };

        struct Image
        {
                std::array<int, N - 1> size;
                // Цветовое пространство sRGB, последовательность red, green, blue, alpha.
                // Каждый цветовой компонент в интервале [0, 255].
                std::vector<unsigned char> srgba_pixels;
        };

        std::vector<Vector<N, float>> vertices;
        std::vector<Vector<N, float>> normals;
        std::vector<Vector<N - 1, float>> texcoords;
        std::vector<Facet> facets;
        std::vector<Point> points;
        std::vector<Line> lines;
        std::vector<Material> materials;
        std::vector<Image> images;
        Vector<N, float> center{0};
        float length{0};

        Mesh() = default;
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&&) = default;
        Mesh& operator=(Mesh&&) = default;
        ~Mesh() = default;
};
}
