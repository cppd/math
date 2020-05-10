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

#include "application_about.h"

#include "../com/support.h"

#include <src/settings/name.h>
#include <src/vulkan/settings.h>

#include <QMessageBox>
#include <sstream>
#include <string>

namespace dialog
{
namespace
{
QString message()
{
        std::ostringstream oss;

        oss << settings::APPLICATION_NAME;

        oss << "\n\n";
#if defined(__clang__)
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
#error Unknown Compiler
#endif

#if !defined(__cplusplus)
#error Unknown C++ Version
#endif
        oss << "\n";
        oss << "__cplusplus " << __cplusplus;

        oss << "\n";

        oss << "\n";
        oss << "Vulkan " << vulkan::API_VERSION_MAJOR << "." << vulkan::API_VERSION_MINOR;

        return QString::fromStdString(oss.str());
}

QString title()
{
        return QString("About ") + settings::APPLICATION_NAME;
}
}

void application_about(QWidget* parent)
{
        QtObjectInDynamicMemory<QMessageBox> w(QMessageBox::NoIcon, title(), message(), QMessageBox::Ok, parent);
        w->exec();
}
}
