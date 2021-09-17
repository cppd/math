/*
Copyright (C) 2017-2021 Topological Manifold

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

namespace ns::gui
{
MainThread::MainThread()
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                error_fatal("MainThread must be called once");
        }

        qRegisterMetaType<std::function<void()>>("std::function<void()>");

        connect(
                this, &MainThread::signal, this,
                [](const std::function<void()>& f)
                {
                        f();
                },
                Qt::AutoConnection);

        main_thread_ = this;
}

MainThread::~MainThread()
{
        main_thread_ = nullptr;
}

void MainThread::run(const std::function<void()>& f)
{
        Q_EMIT main_thread_->signal(f);
}
}
