/*
Copyright (C) 2018 Topological Manifold

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

#include "path_tracing/shapes/mesh.h"
#include "show/show.h"
#include "ui/dialogs/path_tracing_parameters.h"

#include <memory>
#include <string>

void painting(PathTracingParameters&& parameters_window, const IShow& show, const std::shared_ptr<const Mesh>& mesh,
              const std::string& window_title, const std::string& model_name, int default_samples_per_pixel,
              int max_samples_per_pixel, const vec3& background_color, const vec3& default_color, double diffuse);
