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

#include "objects.h"

namespace opengl
{
class TimeElapsed
{
        friend class TimeElapsedRun;

        TimeElapsedQueryHandle m_query;
        bool m_started = false;

        void begin();
        void end();

public:
        ~TimeElapsed();

        double milliseconds() const;
};

class TimeElapsedRun
{
        opengl::TimeElapsed& m_time_elapsed;

public:
        TimeElapsedRun(opengl::TimeElapsed& time_elapsed) : m_time_elapsed(time_elapsed)
        {
                m_time_elapsed.begin();
        }

        ~TimeElapsedRun()
        {
                m_time_elapsed.end();
        }
};
}

#endif
