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

#include "object_selection.h"

#include <src/com/error.h>
#include <src/gui/com/support.h>

#include <QDialog>
#include <QPushButton>

#include <mutex>
#include <optional>
#include <utility>

namespace ns::gui::dialogs
{
namespace
{
class DialogParameters final
{
        mutable std::mutex mutex_;
        ObjectSelectionParameters parameters_{.bound_cocone = true, .cocone = true, .convex_hull = true, .mst = true};

public:
        ObjectSelectionParameters read() const
        {
                const std::lock_guard lg(mutex_);
                return parameters_;
        }

        void write(const ObjectSelectionParameters& parameters)
        {
                const std::lock_guard lg(mutex_);
                parameters_ = parameters;
        }
};

DialogParameters& dialog_parameters()
{
        static DialogParameters parameters;
        return parameters;
}
}

ObjectSelectionParametersDialog::ObjectSelectionParametersDialog(const ObjectSelectionParameters& input)
        : QDialog(com::parent_for_dialog())
{
        ui_.setupUi(this);
        setWindowTitle("Object Selection");

        connect(ui_.push_button_set_all, &QPushButton::clicked, this,
                [this]
                {
                        set_all(true);
                });
        connect(ui_.push_button_clear_all, &QPushButton::clicked, this,
                [this]
                {
                        set_all(false);
                });

        ui_.check_box_bound_cocone->setChecked(input.bound_cocone);
        ui_.check_box_cocone->setChecked(input.cocone);
        ui_.check_box_convex_hull->setChecked(input.convex_hull);
        ui_.check_box_minumum_spanning_tree->setChecked(input.mst);

        com::set_dialog_size(this);
}

void ObjectSelectionParametersDialog::set_all(const bool checked)
{
        ui_.check_box_convex_hull->setChecked(checked);
        ui_.check_box_minumum_spanning_tree->setChecked(checked);
        ui_.check_box_cocone->setChecked(checked);
        ui_.check_box_bound_cocone->setChecked(checked);
}

void ObjectSelectionParametersDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        parameters_.emplace();
        parameters_->bound_cocone = ui_.check_box_bound_cocone->isChecked();
        parameters_->cocone = ui_.check_box_cocone->isChecked();
        parameters_->convex_hull = ui_.check_box_convex_hull->isChecked();
        parameters_->mst = ui_.check_box_minumum_spanning_tree->isChecked();

        QDialog::done(r);
}

std::optional<ObjectSelectionParameters> ObjectSelectionParametersDialog::show()
{
        const com::QtObjectInDynamicMemory w(new ObjectSelectionParametersDialog(dialog_parameters().read()));

        if (w->exec() != QDialog::Accepted || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(w->parameters_);
        dialog_parameters().write(*w->parameters_);

        return std::move(w->parameters_);
}

ObjectSelectionParameters ObjectSelectionParametersDialog::current()
{
        return dialog_parameters().read();
}
}
