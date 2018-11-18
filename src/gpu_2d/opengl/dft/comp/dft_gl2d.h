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

#include "graphics/opengl/objects.h"

#include <complex>
#include <memory>
#include <vector>

struct IFourierGL1
{
        virtual ~IFourierGL1() = default;
        virtual void exec(bool inverse, std::vector<std::complex<float>>* src) = 0;
};

struct IFourierGL2
{
        virtual ~IFourierGL2() = default;
        virtual void exec(bool inverse, bool srgb) = 0;
};

std::unique_ptr<IFourierGL1> create_fft_gl2d(int x, int y);
std::unique_ptr<IFourierGL2> create_fft_gl2d(int x, int y, const opengl::TextureRGBA32F& texture);
