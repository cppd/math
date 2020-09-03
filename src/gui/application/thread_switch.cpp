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

#include "thread_switch.h"

#include <src/com/error.h>

namespace gui::application
{
ThreadSwitch::ThreadSwitch()
{
        qRegisterMetaType<std::function<void()>>("std::function<void()>");

        connect(this, &ThreadSwitch::signal, this, &ThreadSwitch::slot, Qt::QueuedConnection);
}

void ThreadSwitch::slot(const std::function<void()>& f) const
{
        f();
}

void ThreadSwitch::run_in_object_thread(const std::function<void()>& f) const
{
        Q_EMIT signal(f);
}

//

const ThreadSwitch* GlobalThreadSwitch::m_thread_switch_object = nullptr;

GlobalThreadSwitch::GlobalThreadSwitch()
{
        ASSERT(!m_thread_switch_object);
        m_thread_switch_object = &m_thread_switch;
}

GlobalThreadSwitch::~GlobalThreadSwitch()
{
        ASSERT(m_thread_switch_object);
        m_thread_switch_object = nullptr;
}

void GlobalThreadSwitch::run_in_global_thread(const std::function<void()>& f)
{
        ASSERT(m_thread_switch_object);
        m_thread_switch_object->run_in_object_thread(f);
}
}
