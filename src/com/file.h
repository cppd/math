/*
Copyright (C) 2017 Topological Manifold

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

#include "error.h"

#include <cstdio>
#include <string>

class CFile final
{
        FILE* f;

public:
        CFile(const std::string& file_name, const std::string& flags)
        {
                f = fopen(file_name.c_str(), flags.c_str());
                if (!f)
                {
                        error("Error open file " + file_name + " with flags \"" + flags + "\"");
                }
        }

        ~CFile()
        {
                fclose(f);
        }

        operator FILE*() const
        {
                return f;
        }

        CFile(const CFile&) = delete;
        CFile& operator=(const CFile&) = delete;
        CFile(CFile&&) = delete;
        CFile& operator=(CFile&&) = delete;
};
