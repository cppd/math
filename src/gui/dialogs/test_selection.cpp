/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "test_selection.h"

#include "message.h"

#include "../com/support.h"

#include <algorithm>

namespace ns::gui::dialog
{
TestSelectionParametersDialog::TestSelectionParametersDialog(
        std::vector<std::string> test_names,
        std::optional<TestSelectionParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Select Tests");

        std::sort(test_names.begin(), test_names.end());

        for (const std::string& name : test_names)
        {
                std::unique_ptr<QListWidgetItem> item = std::make_unique<QListWidgetItem>(QString::fromStdString(name));
                item->setCheckState(Qt::Checked);
                ui.listWidget->addItem(item.release());
        }

        connect(ui.pushButton_set_all, &QPushButton::clicked, this,
                [this]()
                {
                        set_all(true);
                });
        connect(ui.pushButton_clear_all, &QPushButton::clicked, this,
                [this]()
                {
                        set_all(false);
                });
}

void TestSelectionParametersDialog::set_all(bool checked)
{
        const Qt::CheckState check_state = checked ? Qt::Checked : Qt::Unchecked;

        for (int i = 0, count = ui.listWidget->count(); i < count; ++i)
        {
                ui.listWidget->item(i)->setCheckState(check_state);
        }
}

void TestSelectionParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::vector<std::string> test_names;
        for (int i = 0, count = ui.listWidget->count(); i < count; ++i)
        {
                const QListWidgetItem* item = ui.listWidget->item(i);
                if (!item->isHidden() && item->checkState() == Qt::Checked)
                {
                        test_names.push_back(item->text().toStdString());
                }
        }

        if (test_names.empty())
        {
                dialog::message_critical("No test selected");
                return;
        }

        m_parameters.emplace();
        m_parameters->test_names = std::move(test_names);

        QDialog::done(r);
}

std::optional<TestSelectionParameters> TestSelectionParametersDialog::show(std::vector<std::string> test_names)
{
        std::optional<TestSelectionParameters> parameters;

        QtObjectInDynamicMemory w(new TestSelectionParametersDialog(std::move(test_names), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
