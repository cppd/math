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

#include "object_id.h"
#include "options.h"

#include <string>
#include <unordered_set>

class ObjectStorageEvents
{
protected:
        virtual ~ObjectStorageEvents() = default;

public:
        virtual void file_loaded(
                const std::string& msg,
                unsigned dimension,
                const std::unordered_set<ComputationType>& objects) const = 0;

        virtual void object_loaded(ObjectId id, size_t dimension) const = 0;
        virtual void mesh_loaded(ObjectId id) const = 0;

        virtual void object_deleted(ObjectId id, size_t dimension) const = 0;
        virtual void object_deleted_all(size_t dimension) const = 0;

        virtual void message_warning(const std::string& msg) const = 0;
};
