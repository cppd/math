/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "image_slice.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>

namespace ns::gui::dialog
{
namespace
{
void check_parameters(const std::vector<int>& size, const int slice_dimension)
{
        if (!(size.size() >= 2))
        {
                error("Image dimension " + to_string(size.size()) + " must be greater than or equal to 2");
        }

        if (!all_positive(size))
        {
                error("Image size " + to_string(size) + " must be positive");
        }

        if (!(slice_dimension > 0 && static_cast<std::size_t>(slice_dimension) < size.size()))
        {
                error("Slice dimension " + to_string(slice_dimension) + " must be in the range [1, "
                      + to_string(size.size()) + ")");
        }
}

void create_slider(
        QDialog* const dialog,
        QGridLayout* const layout,
        const int i,
        const int size,
        const int max_size,
        std::optional<int>* const slice)
{
        constexpr bool CHECKED = false;

        auto* const label_d = new QLabel(QString::fromStdString("d[" + to_string(i) + "]"), dialog);
        auto* const label_e = new QLabel("=", dialog);

        auto* const check_box = new QCheckBox(dialog);
        check_box->setChecked(CHECKED);

        auto* const label = new QLabel(dialog);
        set_label_minimum_width_for_text(label, to_string_digit_groups(max_size - 1));
        label->setEnabled(CHECKED);

        auto* const slider = new QSlider(dialog);
        slider->setOrientation(Qt::Horizontal);
        slider->setMinimum(0);
        slider->setMaximum(size - 1);
        slider->setEnabled(CHECKED);
        set_slider_to_middle(slider);
        label->setText(QString::fromStdString(to_string_digit_groups(slider->value())));

        static_assert(!CHECKED);
        slice->reset();

        layout->addWidget(label_d, i, 0);
        layout->addWidget(label_e, i, 1);
        layout->addWidget(check_box, i, 2);
        layout->addWidget(label, i, 3);
        layout->addWidget(slider, i, 4);

        dialog->connect(
                slider, &QSlider::valueChanged, dialog,
                [=]()
                {
                        set_label_text_and_minimum_width(label, to_string_digit_groups(slider->value()));

                        ASSERT(check_box->isChecked());
                        *slice = slider->value();
                });

        dialog->connect(
                check_box, &QCheckBox::stateChanged, dialog,
                [=]()
                {
                        label->setEnabled(check_box->isChecked());
                        slider->setEnabled(check_box->isChecked());

                        if (check_box->isChecked())
                        {
                                *slice = slider->value();
                        }
                        else
                        {
                                slice->reset();
                        }
                });
}
}

ImageSliceDialog::ImageSliceDialog(
        const std::vector<int>& size,
        const int slice_dimension,
        std::optional<ImageSliceParameters>* const parameters)
        : QDialog(parent_for_dialog()),
          slice_dimension_(slice_dimension),
          parameters_(parameters)
{
        check_parameters(size, slice_dimension);

        ui_.setupUi(this);
        setWindowTitle("Image Slice");

        const int max_size = *std::max_element(size.cbegin(), size.cend());

        auto* const widget = new QWidget(this);
        auto* const layout = new QGridLayout(widget);
        layout->setContentsMargins(5, 5, 5, 5);

        slices_.resize(size.size());

        for (std::size_t i = 0; i < size.size(); ++i)
        {
                create_slider(this, layout, i, size[i], max_size, &slices_[i]);
        }

        ASSERT(qobject_cast<QVBoxLayout*>(this->layout()));
        qobject_cast<QVBoxLayout*>(this->layout())->insertWidget(0, widget);

        this->setMinimumWidth(this->fontMetrics().boundingRect(QString(75, 'a')).width());

        set_dialog_height(this);
}

void ImageSliceDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int count = slices_.size();
        for (const std::optional<int>& v : slices_)
        {
                if (v.has_value())
                {
                        --count;
                }
        }

        if (slice_dimension_ != count)
        {
                const std::string msg = "Slice dimension must be equal to " + to_string(slice_dimension_);
                dialog::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        parameters.slices = std::move(slices_);

        QDialog::done(r);
}

std::optional<ImageSliceParameters> ImageSliceDialog::show(const std::vector<int>& size, const int slice_dimension)
{
        std::optional<ImageSliceParameters> parameters;

        const QtObjectInDynamicMemory w(new ImageSliceDialog(size, slice_dimension, &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        return parameters;
}
}
