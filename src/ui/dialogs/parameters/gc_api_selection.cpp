/*
Copyright (C) 2017-2019 Topological Manifold

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
#include "ui/dialogs/messages/message_box.h"
#include "ui/support/support.h"

#include <QPointer>

namespace graphics_and_compute_api_selection_implementation
{
GraphicsAndComputeAPISelection::GraphicsAndComputeAPISelection(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("API");
}

bool GraphicsAndComputeAPISelection::show(GraphicsAndComputeAPI* api)
{
        ASSERT(api);

        std::string vulkan_text = to_string(GraphicsAndComputeAPI::Vulkan);
        std::string opengl_text = to_string(GraphicsAndComputeAPI::OpenGL);

        ui.radio_button_vulkan->setText(vulkan_text.c_str());
        ui.radio_button_opengl->setText(opengl_text.c_str());

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        *api = m_api;

        return true;
}

void GraphicsAndComputeAPISelection::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        bool vulkan = ui.radio_button_vulkan->isChecked();
        bool opengl = ui.radio_button_opengl->isChecked();

        int count = 0;
        count += vulkan ? 1 : 0;
        count += opengl ? 1 : 0;
        if (count > 1)
        {
                std::string msg = "Button error";
                dialog::message_critical(this, msg);
                return;
        }
        else if (vulkan)
        {
                m_api = GraphicsAndComputeAPI::Vulkan;
        }
        else if (opengl)
        {
                m_api = GraphicsAndComputeAPI::OpenGL;
        }
        else
        {
                std::string msg = "Graphics and compute API not selected";
                dialog::message_critical(this, msg);
                return;
        }

        QDialog::done(r);
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
