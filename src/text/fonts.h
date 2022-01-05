/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <map>
#include <string>
#include <vector>

namespace ns::text
{
class Fonts
{
        std::map<std::string_view, std::vector<unsigned char> (*)()> fonts_;

        Fonts();

public:
        static const Fonts& instance();

        std::vector<std::string> names() const;
        std::vector<unsigned char> data(const std::string_view& name) const;
};
}
