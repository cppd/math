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

namespace ns::gui::dialog
{
class TestSelectionParametersDialog::Items final
{
        struct Item final
        {
                std::string name;
                QListWidgetItem* item;
                QString lower_text;

                Item(std::string&& name, QListWidgetItem* item)
                        : name(std::move(name)), item(item), lower_text(item->text().toLower())
                {
                }
        };

        std::vector<Item> m_items;

public:
        explicit Items(std::size_t count)
        {
                m_items.reserve(count);
        }

        void add(std::string&& name, QListWidgetItem* item)
        {
                item->setCheckState(Qt::Checked);
                m_items.emplace_back(std::move(name), item);
        }

        void check(bool checked)
        {
                const Qt::CheckState check_state = checked ? Qt::Checked : Qt::Unchecked;
                for (Item& item : m_items)
                {
                        if (!item.item->isHidden())
                        {
                                item.item->setCheckState(check_state);
                        }
                }
        }

        void filter(const QString& text)
        {
                const QString s = text.simplified().toLower();
                const QVector<QStringRef> filter = s.splitRef(' ', Qt::SkipEmptyParts);
                for (Item& item : m_items)
                {
                        bool contains = true;
                        for (const QStringRef& ref : filter)
                        {
                                if (!item.lower_text.contains(ref, Qt::CaseSensitive))
                                {
                                        contains = false;
                                        break;
                                }
                        }
                        item.item->setHidden(!contains);
                }
        }

        std::vector<std::string> selected() const
        {
                std::vector<std::string> names;
                for (const Item& item : m_items)
                {
                        if (!item.item->isHidden() && item.item->checkState() == Qt::Checked)
                        {
                                names.push_back(item.name);
                        }
                }
                return names;
        }
};

TestSelectionParametersDialog::TestSelectionParametersDialog(
        std::vector<std::string> test_names,
        std::optional<TestSelectionParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Select Tests");

        std::sort(test_names.begin(), test_names.end());

        m_items = std::make_unique<Items>(test_names.size());

        for (std::string& name : test_names)
        {
                std::unique_ptr<QListWidgetItem> item = std::make_unique<QListWidgetItem>(QString::fromStdString(name));
                m_items->add(std::move(name), item.get());
                ui.listWidget->addItem(item.release());
        }

        connect(ui.pushButton_set_all, &QPushButton::clicked, this,
                [this]()
                {
                        m_items->check(true);
                });

        connect(ui.pushButton_clear_all, &QPushButton::clicked, this,
                [this]()
                {
                        m_items->check(false);
                });

        connect(ui.lineEdit_filter, &QLineEdit::textChanged, this,
                [this](const QString& text)
                {
                        m_items->filter(text);
                });
}

TestSelectionParametersDialog::~TestSelectionParametersDialog() = default;

void TestSelectionParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::vector<std::string> test_names = m_items->selected();

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
