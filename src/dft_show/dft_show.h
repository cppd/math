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
#ifndef DFT_SHOW_H
#define DFT_SHOW_H

#include "gl/gl_objects.h"

#include <glm/mat4x4.hpp>
#include <memory>

class DFTShow final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        DFTShow(int width, int height, int pos_x, int pos_y, const glm::mat4& mtx, bool source_sRGB, const TextureRGBA32F& tex);
        ~DFTShow();

        void set_brightness(float brightness);

        void draw();
};

#endif
