/*
Copyright (C) 2017-2019 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include "graphics/opengl/buffers.h"

#include <complex>
#include <memory>
#include <vector>

namespace gpu_opengl
{
struct DFTCompute
{
        virtual ~DFTCompute() = default;
        virtual void exec(bool inverse, std::vector<std::complex<float>>* src) = 0;
};

struct DFTComputeTexture
{
        virtual ~DFTComputeTexture() = default;
        virtual void exec() = 0;
};

std::unique_ptr<DFTCompute> create_dft_compute(unsigned width, unsigned height);
std::unique_ptr<DFTComputeTexture> create_dft_compute_texture(const opengl::Texture& source, unsigned x, unsigned y,
                                                              unsigned width, unsigned height, const opengl::Texture& result);
}

#endif
