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

#include "thread_ui.h"

#include <src/com/error.h>

namespace application
{
namespace
{
ThreadUI* g_thread_ui_object = nullptr;
}

ThreadUI::ThreadUI()
{
        qRegisterMetaType<std::function<void()>>("std::function<void()>");

        connect(this, SIGNAL(object_signal(const std::function<void()>&)), this,
                SLOT(object_slot(const std::function<void()>&)));

        ASSERT(!g_thread_ui_object);
        g_thread_ui_object = this;
}

ThreadUI::~ThreadUI()
{
        ASSERT(g_thread_ui_object);
        g_thread_ui_object = nullptr;
}

void ThreadUI::object_slot(const std::function<void()>& f) const
{
        f();
}

void ThreadUI::run(const std::function<void()>& f) const
{
        emit object_signal(f);
}

void ThreadUI::run_in_ui_thread(const std::function<void()>& f)
{
        ASSERT(g_thread_ui_object);
        g_thread_ui_object->run(f);
}
}
