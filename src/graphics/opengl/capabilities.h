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

#include "com/error.h"
#include "graphics/opengl/functions/opengl_functions.h"

#include <array>

namespace opengl
{
template <GLenum... E>
class GLEnableAndRestore
{
        static_assert(sizeof...(E) > 0);
        std::array<bool, sizeof...(E)> m_enabled;

        static bool get_errors()
        {
                while (glGetError() != GL_NO_ERROR)
                {
                }
                return true;
        }

public:
        GLEnableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] = glIsEnabled(E), glEnable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
        ~GLEnableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] ? glEnable(E) : glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
};

template <GLenum... E>
class GLDisableAndRestore
{
        static_assert(sizeof...(E) > 0);
        std::array<bool, sizeof...(E)> m_enabled;

        static bool get_errors()
        {
                while (glGetError() != GL_NO_ERROR)
                {
                }
                return true;
        }

public:
        GLDisableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] = glIsEnabled(E), glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
        ~GLDisableAndRestore()
        {
                ASSERT(get_errors());
                size_t i = 0;
                ((m_enabled[i++] ? glEnable(E) : glDisable(E)), ...);
                ASSERT(glGetError() == GL_NO_ERROR);
        }
};
}
