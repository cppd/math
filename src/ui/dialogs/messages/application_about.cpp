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

#include <QMessageBox>
#include <sstream>
#include <string>

namespace
{
std::string message()
{
        std::ostringstream oss;

        oss << APPLICATION_NAME << "\n\n";

        oss << "Compiled by ";
#if defined(__clang__)
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
#error Unknown Compiler
#endif
        oss << "\n\n";

        oss << "Languages:\n        C++17, GLSL 4.50.\n";
#if defined(__linux__)
        oss << "Libraries:\n        Freetype, GMP, OpenGL, Qt, SFML, X11.";
#elif defined(_WIN32)
        oss << "Libraries:\n        Freetype, GMP, OpenGL, Qt, SFML.";
#else
#error This operating system is not supported
#endif

        return oss.str();
}

std::string title()
{
        return std::string("About ") + APPLICATION_NAME;
}
}

void application_about(QWidget* parent)
{
        static const QString m = message().c_str();
        static const QString t = title().c_str();
        QMessageBox::about(parent, t, m);
}
