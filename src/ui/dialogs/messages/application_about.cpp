/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "application_about.h"

#include "application/application_name.h"
#include "ui/support/support.h"

#include <QMessageBox>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

constexpr const char* LANGUAGES[] = {"C++17", "GLSL 4.50"};

constexpr const char* LIBRARIES[] = {"FreeType",
                                     "GLFW",
                                     "GMP",
                                     "OpenGL",
                                     "Qt",
                                     "SFML",
                                     "Vulkan"
#if defined(__linux__)
                                     ,
                                     "Xlib"
#endif
};

namespace
{
template <typename Iterator>
void sorted_comma_separated_list(std::ostringstream& oss, Iterator first, Iterator last)
{
        std::vector<std::string> v(first, last);

        if (v.size() == 0)
        {
                return;
        }

        std::sort(v.begin(), v.end());

        oss << v[0];
        for (unsigned i = 1; i < v.size(); ++i)
        {
                oss << ", " << v[i];
        }
}

std::string message()
{
        std::ostringstream oss;

        oss << APPLICATION_NAME;

        oss << "\n\n";
#if defined(__clang__)
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
#error Unknown Compiler
#endif

        oss << "\n\n";
        sorted_comma_separated_list(oss, std::cbegin(LANGUAGES), std::cend(LANGUAGES));
        oss << ".";

        oss << "\n\n";
        sorted_comma_separated_list(oss, std::cbegin(LIBRARIES), std::cend(LIBRARIES));
        oss << ".";

        return oss.str();
}

std::string title()
{
        return std::string("About ") + APPLICATION_NAME;
}
}

namespace dialog
{
void application_about(QWidget* parent)
{
        static const QString t = title().c_str();
        static const QString m = message().c_str();

        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::NoIcon, t, m, QMessageBox::Ok, parent);
        w->exec();
}
}
