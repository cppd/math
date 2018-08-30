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

#include "gc_api_selection.h"

#include "com/error.h"
#include "ui/support/support.h"

#include <QPointer>

namespace graphics_and_compute_api_selection_implementation
{
GraphicsAndComputeAPISelection::GraphicsAndComputeAPISelection(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Graphics And Compute API Selection");
}

bool GraphicsAndComputeAPISelection::show(GraphicsAndComputeAPI* api)
{
        ASSERT(api);

        if (!(*api == GraphicsAndComputeAPI::Vulkan || *api == GraphicsAndComputeAPI::OpenGL))
        {
                error_fatal("Default graphics and compute API is neither Vulkan nor OpenGL");
        }

        std::string vulkan_comment = graphics_and_compute_api_comment(GraphicsAndComputeAPI::Vulkan);
        std::string opengl_comment = graphics_and_compute_api_comment(GraphicsAndComputeAPI::OpenGL);

        std::string vulkan_text =
                to_string(GraphicsAndComputeAPI::Vulkan) + (vulkan_comment.size() > 0 ? " (" + vulkan_comment + ")" : "");
        std::string opengl_text =
                to_string(GraphicsAndComputeAPI::OpenGL) + (opengl_comment.size() > 0 ? " (" + opengl_comment + ")" : "");

        ui.radio_button_vulkan->setText(vulkan_text.c_str());
        ui.radio_button_opengl->setText(opengl_text.c_str());

        ui.radio_button_vulkan->setChecked(*api == GraphicsAndComputeAPI::Vulkan);
        ui.radio_button_opengl->setChecked(*api == GraphicsAndComputeAPI::OpenGL);

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        if (ui.radio_button_vulkan->isChecked() && !ui.radio_button_opengl->isChecked())
        {
                *api = GraphicsAndComputeAPI::Vulkan;
        }
        else if (!ui.radio_button_vulkan->isChecked() && ui.radio_button_opengl->isChecked())
        {
                *api = GraphicsAndComputeAPI::OpenGL;
        }
        else
        {
                error_fatal("Failed to select graphics and compute API");
        }

        return true;
}
}

namespace dialog
{
bool graphics_and_compute_api_selection(QWidget* parent, GraphicsAndComputeAPI* api)
{
        QtObjectInDynamicMemory<graphics_and_compute_api_selection_implementation::GraphicsAndComputeAPISelection> w(parent);
        return w->show(api);
}
}
