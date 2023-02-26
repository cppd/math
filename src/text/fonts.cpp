/*
Copyright (C) 2017-2023 Topological Manifold

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
std::vector<unsigned char> deja_vu_sans()
{
        static constexpr unsigned char DATA[] = {
#include "DejaVuSans.ttf.bin"
        };
        return {std::cbegin(DATA), std::cend(DATA)};
}
}

const Fonts& Fonts::instance()
{
        static const Fonts fonts;
        return fonts;
}

Fonts::Fonts()
{
        fonts_.emplace("DejaVuSans", deja_vu_sans);
}

std::vector<std::string> Fonts::names() const
{
        std::vector<std::string> res;
        res.reserve(fonts_.size());
        for (const auto& e : fonts_)
        {
                res.emplace_back(e.first);
        }
        return res;
}

std::vector<unsigned char> Fonts::data(const std::string_view name) const
{
        const auto iter = fonts_.find(name);
        if (iter != fonts_.cend())
        {
                return iter->second();
        }
        error("Font not found: " + std::string(name));
}
}
