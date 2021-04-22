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

#pragma once

#include "ui_test_selection.h"

#include <string>
#include <vector>

namespace ns::gui::dialog
{
struct TestSelectionParameters final
{
        std::vector<std::string> test_names;
};

class TestSelectionParametersDialog final : public QDialog
{
        Q_OBJECT
private:
        Ui::TestSelectionParametersDialog ui;

        std::optional<TestSelectionParameters>& m_parameters;

        TestSelectionParametersDialog(
                std::vector<std::string> test_names,
                std::optional<TestSelectionParameters>& parameters);

        void set_all(bool checked);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<TestSelectionParameters> show(std::vector<std::string> test_names);
};
}
