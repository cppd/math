/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/view/event.h>

#include <vector>

namespace ns::view::com
{
class View
{
public:
        virtual ~View() = default;

        virtual void render() = 0;

        virtual void exec(const std::vector<Command>& commands) = 0;
        virtual void receive(const std::vector<Info>& infos) = 0;
};
}
