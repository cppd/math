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

#include "objects.h"
#include "path_tracing/shapes/mesh.h"

#include <memory>

std::unique_ptr<const PaintObjects> cornell_box(int width, int height, const std::string& obj_file_name, double size,
                                                const vec3& default_color, double diffuse, const vec3& camera_direction,
                                                const vec3& camera_up, int samples_per_pixel);

std::unique_ptr<const PaintObjects> cornell_box(int width, int height, const std::shared_ptr<const Mesh>& mesh, double size,
                                                const vec3& default_color, double diffuse, const vec3& camera_direction,
                                                const vec3& camera_up, int samples_per_pixel);

std::unique_ptr<const PaintObjects> one_object_scene(const vec3& background_color, const vec3& default_color, double diffuse,
                                                     std::unique_ptr<const Projector>&& projector,
                                                     std::unique_ptr<const Sampler2d>&& sampler,
                                                     std::unique_ptr<const LightSource>&& light_source,
                                                     const std::shared_ptr<const Mesh>& mesh);
