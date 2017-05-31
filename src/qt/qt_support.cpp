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

#include "qt_support.h"

#include "com/error.h"

#include <QButtonGroup>
#include <QWidget>

namespace
{
bool is_child_widget_of_any_layout(QLayout* layout, QWidget* widget)
{
        if (layout == nullptr or widget == nullptr)
        {
                return false;
        }
        if (layout->indexOf(widget) >= 0)
        {
                return true;
        }
        foreach (QObject* o, layout->children())
        {
                if (is_child_widget_of_any_layout(static_cast<QLayout*>(o), widget))
                {
                        return true;
                }
        }
        return false;
}
}

void set_widgets_enabled(QLayout* layout, bool v)
{
        ASSERT(layout);

        QWidget* pw = layout->parentWidget();
        if (!pw)
        {
                return;
        }
        foreach (QWidget* w, pw->findChildren<QWidget*>())
        {
                if (is_child_widget_of_any_layout(layout, w))
                {
                        w->setEnabled(v);
                }
        }
}

glm::vec3 qcolor_to_vec3(const QColor& c)
{
        return glm::vec3(c.redF(), c.greenF(), c.blueF());
}

void button_strike_out(QRadioButton* button, bool strike_out)
{
        ASSERT(button);

        QFont f = button->font();
        f.setStrikeOut(strike_out);
        button->setFont(f);
}

void disable_radio_button(QRadioButton* button)
{
        ASSERT(button && button->group());

        button->group()->setExclusive(false);
        // button->setAutoExclusive(false);
        button->setChecked(false);
        // button->setAutoExclusive(true);
        button->setEnabled(false);
        button->group()->setExclusive(true);
}
