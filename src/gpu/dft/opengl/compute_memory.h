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

#pragma once

#include "graphics/opengl/buffers.h"

namespace gpu_opengl
{
template <typename T>
class DftMemoryFftGlobalData final
{
        static constexpr int DATA_BINDING = 0;

        struct Data
        {
                GLuint m_div_2;
                T two_pi_div_m;
        };

        opengl::Buffer m_data;

public:
        DftMemoryFftGlobalData(T two_pi_div_m, int m_div_2);
        void bind() const;
};

template <typename T>
class DftMemoryFftGlobalBuffer final
{
        static constexpr int BUFFER_BINDING = 1;

        GLuint m_buffer = 0;

public:
        void set(const opengl::Buffer& buffer);
        void bind() const;
};
}
