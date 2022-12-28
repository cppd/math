/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/error.h>

#include <algorithm>
#include <regex>
#include <sstream>

namespace ns::gui::dialog
{
namespace
{
void set_window_size(QDialog* const dialog)
{
        const QSize size = dialog->screen()->geometry().size();
        if (size.width() > size.height())
        {
                const auto v = size.height() / 2;
                dialog->resize(v, v);
        }
        else
        {
                const auto v = size.width() / 2;
                dialog->resize(v, v);
        }
}
}

class TestSelectionParametersDialog::Items final
{
        struct Item final
        {
                std::string name;
                QListWidgetItem* item;
                std::string lower_text;

                Item(std::string&& name, QListWidgetItem* const item)
                        : name(std::move(name)),
                          item(item),
                          lower_text(item->text().toLower().toStdString())
                {
                }
        };

        std::vector<Item> items_;

public:
        explicit Items(const std::size_t count)
        {
                items_.reserve(count);
        }

        void add(std::string&& name, QListWidgetItem* const item)
        {
                item->setCheckState(Qt::Checked);
                items_.emplace_back(std::move(name), item);
        }

        void check(const bool checked)
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

        [[nodiscard]] std::vector<std::string> selected() const
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
        const std::string_view title,
        std::vector<std::string> test_names,
        std::optional<TestSelectionParameters>& parameters)
        : QDialog(parent_for_dialog()),
          parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle(QString::fromUtf8(title.data(), title.size()));

        std::sort(test_names.begin(), test_names.end());
        items_ = std::make_unique<Items>(test_names.size());
        for (std::string& name : test_names)
        {
                auto item = std::make_unique<QListWidgetItem>(QString::fromStdString(name));
                items_->add(std::move(name), item.get());
                ui_.listWidget->addItem(item.release());
        }

        connect(ui_.push_button_set_all, &QPushButton::clicked, this,
                [this]()
                {
                        items_->check(true);
                });

        connect(ui_.push_button_clear_all, &QPushButton::clicked, this,
                [this]()
                {
                        items_->check(false);
                });

        connect(ui_.line_edit_filter, &QLineEdit::textChanged, this,
                [this](const QString& text)
                {
                        filter(text);
                });

        ui_.check_box_regex->setChecked(true);
        connect(ui_.check_box_regex, &QCheckBox::stateChanged, this,
                [this]()
                {
                        filter(ui_.line_edit_filter->text());
                });

        set_window_size(this);
}

TestSelectionParametersDialog::~TestSelectionParametersDialog() = default;

void TestSelectionParametersDialog::filter(const QString& text)
{
        const char* style = nullptr;
        if (ui_.check_box_regex->isChecked())
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
        ui_.line_edit_filter->setStyleSheet(style);
}

void TestSelectionParametersDialog::done(const int r)
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
        const std::string_view title,
        std::vector<std::string> test_names)
{
        std::optional<TestSelectionParameters> parameters;

        const QtObjectInDynamicMemory w(new TestSelectionParametersDialog(title, std::move(test_names), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        return parameters;
}
}
