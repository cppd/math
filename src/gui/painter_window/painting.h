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
#include <src/painter/shapes/mesh.h>

#include <QWidget>
#include <memory>
#include <string>

template <typename T>
struct PaintingInformation3d
{
        Vector<3, T> camera_up;
        Vector<3, T> camera_direction;
        Vector<3, T> light_direction;
        Vector<3, T> object_position;
        T object_size;
        Vector<3, T> view_center;
        T view_width;
        int paint_width;
        int paint_height;
        int max_screen_size;
};

struct PaintingInformationNd
{
        int default_screen_size;
        int minimum_screen_size;
        int maximum_screen_size;
};

struct PaintingInformationAll
{
        QWidget* parent_window;
        std::string window_title;
        std::string object_name;
        int default_samples_per_dimension;
        int max_samples_per_dimension;
        Color background_color;
        Color default_color;
        Color::DataType diffuse;
};

template <typename T>
void painting(
        const std::shared_ptr<const SpatialMeshModel<3, T>>& mesh,
        const PaintingInformation3d<T>& info_3d,
        const PaintingInformationAll& info_all);

template <size_t N, typename T>
void painting(
        const std::shared_ptr<const SpatialMeshModel<N, T>>& mesh,
        const PaintingInformationNd& info_nd,
        const PaintingInformationAll& info_all);
