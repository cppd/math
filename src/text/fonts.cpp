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

#include "fonts.h"

#include <src/com/error.h>

namespace text
{
namespace
{
constexpr unsigned char DEJA_VU_SANS[]{
#include "DejaVuSans.ttf.bin"
};

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
        m_fonts.emplace(
                "DejaVuSans",
                []()
                {
                        return std::vector(std::cbegin(DEJA_VU_SANS), std::cend(DEJA_VU_SANS));
                });
}

std::vector<std::string> Fonts::names() const
{
        return names_of_map(m_fonts);
}

std::vector<unsigned char> Fonts::data(const std::string& name) const
{
        auto iter = m_fonts.find(name);
        if (iter != m_fonts.cend())
        {
                return iter->second();
        }
        error("Font not found: " + name);
}
}
