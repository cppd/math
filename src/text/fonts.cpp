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

#include "fonts.h"

#include <src/com/error.h>

namespace ns::text
{
namespace
{
template <typename T>
std::vector<std::string> names_of_map(const std::map<std::string, T>& map)
{
        std::vector<std::string> names;
        names.reserve(map.size());

        for (const auto& e : map)
        {
                names.push_back(e.first);
        }

        return names;
}
}

const Fonts& Fonts::instance()
{
        static const Fonts fonts;
        return fonts;
}

Fonts::Fonts()
{
        fonts_.emplace(
                "DejaVuSans",
                []()
                {
                        static constexpr unsigned char FONT[] = {
#include "DejaVuSans.ttf.bin"
                        };
                        return std::vector<unsigned char>(std::cbegin(FONT), std::cend(FONT));
                });
}

std::vector<std::string> Fonts::names() const
{
        return names_of_map(fonts_);
}

std::vector<unsigned char> Fonts::data(const std::string& name) const
{
        auto iter = fonts_.find(name);
        if (iter != fonts_.cend())
        {
                return iter->second();
        }
        error("Font not found: " + name);
}
}
