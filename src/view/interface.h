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

#pragma once

#include "event.h"

#include <src/model/object_id.h>

#include <string>
#include <vector>

class ViewEvents
{
protected:
        virtual ~ViewEvents() = default;

public:
        virtual void message_error_fatal(const std::string&) const = 0;
        virtual void message_error_source(const std::string&, const std::string&) const = 0;
        virtual void view_object_loaded(ObjectId) const = 0;
};

class View
{
public:
        virtual ~View() = default;

        virtual void send(ViewEvent&&) = 0;
        virtual void receive(const std::vector<ViewInfo*>& info) = 0;
};
