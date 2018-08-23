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

#include "com/color/colors.h"
#include "show/show.h"
#include "window/window_handle.h"

#include <memory>

std::unique_ptr<IShow> create_show_opengl(IShowCallback* callback, WindowID parent_window, double parent_window_dpi,
                                          const Color& background_color_rgb, const Color& default_color_rgb,
                                          const Color& wireframe_color_rgb, bool with_smooth, bool with_wireframe,
                                          bool with_shadow, bool with_fog, bool with_materials, bool with_effect, bool with_dft,
                                          bool with_convex_hull, bool with_optical_flow, double ambient, double diffuse,
                                          double specular, double dft_brightness, const Color& dft_color,
                                          const Color& dft_background_color, double default_ns, bool vertical_sync,
                                          double shadow_zoom);
