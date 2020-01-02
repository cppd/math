/*
Copyright (C) 2017-2020 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include "time.h"

#include "functions.h"

#include "com/error.h"

namespace opengl
{
TimeElapsed::~TimeElapsed()
{
        ASSERT(!m_started);
}

void TimeElapsed::begin()
{
        ASSERT(!m_started);
        glBeginQuery(GL_TIME_ELAPSED, m_query);
        m_started = true;
}

void TimeElapsed::end()
{
        ASSERT(m_started);
        m_started = false;
        glEndQuery(GL_TIME_ELAPSED);
}

double TimeElapsed::milliseconds() const
{
        ASSERT(!m_started);
        GLuint64 nanoseconds;
        glGetQueryObjectui64v(m_query, GL_QUERY_RESULT, &nanoseconds);
        return 1e-6 * nanoseconds;
}
}

#endif
