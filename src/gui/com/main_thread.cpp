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

#include "main_thread.h"

#include <src/com/error.h>

#include <atomic>

namespace gui
{
namespace
{
std::atomic_int global_call_counter = 0;
const MainThread* global_main_thread = nullptr;
}

MainThread::MainThread()
{
        if (++global_call_counter != 1)
        {
                error_fatal("MainThread must be called once");
        }

        qRegisterMetaType<std::function<void()>>("std::function<void()>");

        connect(this, &MainThread::signal, this, &MainThread::slot, Qt::AutoConnection);

        global_main_thread = this;
}

MainThread::~MainThread()
{
        global_main_thread = nullptr;
}

void MainThread::slot(const std::function<void()>& f) const
{
        f();
}

void MainThread::run(const std::function<void()>& f)
{
        Q_EMIT global_main_thread->signal(f);
}
}
