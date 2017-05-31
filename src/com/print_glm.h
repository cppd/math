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
#ifndef PRINT_GLM_H
#define PRINT_GLM_H

#include "print.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

inline std::string to_string(const glm::vec2& data)
{
        return to_string(std::array<float, 2>{{data[0], data[1]}});
}

inline std::string to_string(const glm::vec3& data)
{
        return to_string(std::array<float, 3>{{data[0], data[1], data[2]}});
}

inline std::string to_string(const glm::vec4& data)
{
        return to_string(std::array<float, 4>{{data[0], data[1], data[2], data[3]}});
}

#endif
