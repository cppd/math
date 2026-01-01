/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/gui/com/support.h>
#include <src/settings/name.h>
#include <src/vulkan/api_version.h>

#include <QMessageBox>
#include <QString>

#include <sstream>

namespace ns::gui::dialogs
{
namespace
{
QString message(const bool ray_tracing)
{
        std::ostringstream oss;

        oss << settings::APPLICATION_NAME;

#if defined(BUILD_DEBUG) && defined(BUILD_RELEASE)
#error BUILD_DEBUG and BUILD_RELEASE
#elifdef BUILD_DEBUG
        oss << "\n\n";
        oss << "Debug Build";
#elifdef BUILD_RELEASE
        oss << "\n\n";
        oss << "Release Build";
#endif

        oss << "\n\n";
#ifdef __clang__
        oss << "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elifdef __GNUC__
        oss << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
#error Unknown Compiler
#endif

#ifndef __cplusplus
#error Unknown C++ Version
#else
        oss << "\n";
        oss << "__cplusplus " << __cplusplus;
#endif

#ifdef BUILD_LIB_CPP
        oss << "\n";
        oss << "libc++";
#endif

        oss << "\n\n";
        oss << "Vulkan " << vulkan::API_VERSION_MAJOR << "." << vulkan::API_VERSION_MINOR;
        if (ray_tracing)
        {
                oss << ", Ray Tracing";
        }

        return QString::fromStdString(oss.str());
}

QString title()
{
        return QString("About ") + settings::APPLICATION_NAME;
}
}

void application_about(const bool ray_tracing)
{
        const com::QtObjectInDynamicMemory<QMessageBox> w(
                QMessageBox::NoIcon, title(), message(ray_tracing), QMessageBox::Ok, com::parent_for_dialog());

        w->exec();
}
}
