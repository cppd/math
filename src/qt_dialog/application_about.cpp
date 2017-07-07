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

#include "application_about.h"

#include "application/application_name.h"

#include <QMessageBox>

void application_about(QWidget* parent)
{
        static const QString message =
                "Languages:\n"
                "       C++17, GLSL 4.50.\n"
#if defined(__linux__)
                "Libraries:\n"
                "       Freetype, GLM, GMP, OpenGL, Qt, SFML, X11.";
#elif defined(_WIN32)
                "Libraries:\n"
                "       Freetype, GLM, GMP, OpenGL, Qt, SFML.";
#else
#error This operating system is not supported
#endif

        static const QString title = QString("About ") + APPLICATION_NAME;

        QMessageBox::about(parent, title, message);
}
