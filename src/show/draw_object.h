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
#ifndef DRAW_OBJECT_H
#define DRAW_OBJECT_H

#include "color/color_space.h"
#include "obj/obj.h"

#include <glm/mat4x4.hpp>
#include <memory>

// Несмоторя на абстрактный класс, это непосредственно связано с программами шейдеров
struct IDrawObject
{
        virtual ~IDrawObject() = default;

        virtual const glm::mat4& get_model_matrix() const = 0;
        virtual unsigned get_vertices_count() const = 0;
        virtual void bind_vertex_array() const = 0;
        virtual void bind_storage_buffer(unsigned binding_point) const = 0;
};

std::unique_ptr<IDrawObject> create_draw_object(const std::shared_ptr<IObj>& obj_ptr, const ColorSpaceConverter& color_converter);

#endif
