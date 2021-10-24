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

#include <regex>
#include <sstream>

namespace ns::gui::dialog
{
class TestSelectionParametersDialog::Items final
{
        struct Item final
        {
                std::string name;
                QListWidgetItem* item;
                std::string lower_text;

                Item(std::string&& name, QListWidgetItem* item)
                        : name(std::move(name)), item(item), lower_text(item->text().toLower().toStdString())
                {
                }
        };

        std::vector<Item> items_;

public:
        explicit Items(std::size_t count)
        {
                items_.reserve(count);
        }

        void add(std::string&& name, QListWidgetItem* item)
        {
                item->setCheckState(Qt::Checked);
                items_.emplace_back(std::move(name), item);
        }

        void check(bool checked)
        {
                const Qt::CheckState check_state = checked ? Qt::Checked : Qt::Unchecked;
                for (Item& item : items_)
                {
                        if (!item.item->isHidden())
                        {
                                item.item->setCheckState(check_state);
                        }
                }
        }

        bool filter_regex(const QString& text)
        {
                bool result = true;
                const std::vector<std::regex> regex = [&]
                {
                        std::vector<std::regex> r;
                        std::istringstream iss(text.simplified().toLower().toStdString());
                        std::string s;
                        while (iss >> s)
                        {
                                try
                                {
                                        r.emplace_back(std::move(s));
                                }
                                catch (...)
                                {
                                        result = false;
                                }
                        }
                        return r;
                }();
                for (Item& item : items_)
                {
                        bool contains = true;
                        for (const std::regex& r : regex)
                        {
                                if (!std::regex_search(item.lower_text, r))
                                {
                                        contains = false;
                                        break;
                                }
                        }
                        item.item->setHidden(!contains);
                }
                return result;
        }

        void filter_substr(const QString& text)
        {
                const std::vector<std::string> substr = [&]
                {
                        std::vector<std::string> r;
                        std::istringstream iss(text.simplified().toLower().toStdString());
                        std::string s;
                        while (iss >> s)
                        {
                                r.push_back(std::move(s));
                        }
                        return r;
                }();
                for (Item& item : items_)
                {
                        bool contains = true;
                        for (const std::string& s : substr)
                        {
                                if (item.lower_text.find(s) == std::string::npos)
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
                for (const Item& item : items_)
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
        const std::string_view& title,
        std::vector<std::string> test_names,
        std::optional<TestSelectionParameters>& parameters)
        : QDialog(parent_for_dialog()), parameters_(parameters)
{
        ui_.setupUi(this);

        setWindowTitle(QString::fromUtf8(title.data(), title.size()));

        std::sort(test_names.begin(), test_names.end());
        items_ = std::make_unique<Items>(test_names.size());
        for (std::string& name : test_names)
        {
                std::unique_ptr<QListWidgetItem> item = std::make_unique<QListWidgetItem>(QString::fromStdString(name));
                items_->add(std::move(name), item.get());
                ui_.listWidget->addItem(item.release());
        }

        connect(ui_.pushButton_set_all, &QPushButton::clicked, this,
                [this]()
                {
                        items_->check(true);
                });

        connect(ui_.pushButton_clear_all, &QPushButton::clicked, this,
                [this]()
                {
                        items_->check(false);
                });

        connect(ui_.lineEdit_filter, &QLineEdit::textChanged, this,
                [this](const QString& text)
                {
                        filter(text);
                });

        ui_.checkBox_regex->setChecked(true);
        connect(ui_.checkBox_regex, &QCheckBox::stateChanged, this,
                [this]()
                {
                        filter(ui_.lineEdit_filter->text());
                });
}

TestSelectionParametersDialog::~TestSelectionParametersDialog() = default;

void TestSelectionParametersDialog::filter(const QString& text)
{
        const char* style = nullptr;
        if (ui_.checkBox_regex->isChecked())
        {
                if (items_->filter_regex(text))
                {
                        style = "color: black;";
                }
                else
                {
                        style = "color: red;";
                }
        }
        else
        {
                items_->filter_substr(text);
                style = "color: black;";
        }
        ui_.lineEdit_filter->setStyleSheet(style);
}

void TestSelectionParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        std::vector<std::string> test_names = items_->selected();

        if (test_names.empty())
        {
                dialog::message_critical("No items selected");
                return;
        }

        parameters_.emplace();
        parameters_->test_names = std::move(test_names);

        QDialog::done(r);
}

std::optional<TestSelectionParameters> TestSelectionParametersDialog::show(
        const std::string_view& title,
        std::vector<std::string> test_names)
{
        std::optional<TestSelectionParameters> parameters;

        QtObjectInDynamicMemory w(new TestSelectionParametersDialog(title, std::move(test_names), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
