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

#include "image_slice.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>

namespace ns::gui::dialog
{
ImageSliceDialog::ImageSliceDialog(
        const std::vector<int>& size,
        int slice_dimension,
        std::optional<ImageSliceParameters>& parameters)
        : QDialog(parent_for_dialog()), m_slice_dimension(slice_dimension), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Image Slice");

        if (!(size.size() >= 2))
        {
                error("Image dimension " + to_string(size.size()) + " must be greater than or equal to 2");
        }

        if (!std::all_of(
                    size.cbegin(), size.cend(),
                    [](const int& s)
                    {
                            return s > 0;
                    }))
        {
                error("Image size " + to_string(size) + " must be positive");
        }

        if (!(slice_dimension > 0 && static_cast<std::size_t>(slice_dimension) < size.size()))
        {
                error("Slice dimension " + to_string(slice_dimension) + " must be in the range [1, "
                      + to_string(size.size()) + ")");
        }

        const int max_size = *std::max_element(size.cbegin(), size.cend());

        QWidget* widget = new QWidget(this);
        QGridLayout* layout = new QGridLayout(widget);
        layout->setContentsMargins(5, 5, 5, 5);

        m_slices.resize(size.size());

        for (std::size_t i = 0; i < size.size(); ++i)
        {
                constexpr bool CHECKED = false;

                QLabel* const label_d = new QLabel(QString::fromStdString("d[" + to_string(i) + "]"), this);
                QLabel* const label_e = new QLabel("=", this);

                QCheckBox* const check_box = new QCheckBox(this);
                check_box->setChecked(CHECKED);

                QLabel* const label = new QLabel(this);
                set_label_minimum_width_for_text(label, to_string_digit_groups(max_size - 1));
                label->setEnabled(CHECKED);

                QSlider* const slider = new QSlider(this);
                slider->setOrientation(Qt::Horizontal);
                slider->setMinimum(0);
                slider->setMaximum(size[i] - 1);
                slider->setEnabled(CHECKED);
                set_slider_to_middle(slider);
                label->setText(QString::fromStdString(to_string_digit_groups(slider->value())));

                static_assert(!CHECKED);
                m_slices[i].reset();

                layout->addWidget(label_d, i, 0);
                layout->addWidget(label_e, i, 1);
                layout->addWidget(check_box, i, 2);
                layout->addWidget(label, i, 3);
                layout->addWidget(slider, i, 4);

                connect(slider, &QSlider::valueChanged, this,
                        [=, this]()
                        {
                                set_label_text_and_minimum_width(label, to_string_digit_groups(slider->value()));

                                ASSERT(check_box->isChecked());
                                m_slices[i] = slider->value();
                        });

                connect(check_box, &QCheckBox::stateChanged, this,
                        [=, this]()
                        {
                                label->setEnabled(check_box->isChecked());
                                slider->setEnabled(check_box->isChecked());

                                if (check_box->isChecked())
                                {
                                        m_slices[i] = slider->value();
                                }
                                else
                                {
                                        m_slices[i].reset();
                                }
                        });
        }

        ASSERT(qobject_cast<QVBoxLayout*>(this->layout()));
        qobject_cast<QVBoxLayout*>(this->layout())->insertWidget(0, widget);

        this->setMinimumWidth(500);
        this->adjustSize();
}

void ImageSliceDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int count = m_slices.size();
        for (const std::optional<int>& v : m_slices)
        {
                if (v.has_value())
                {
                        --count;
                }
        }

        if (m_slice_dimension != count)
        {
                std::string msg = "Slice dimension must be equal to " + to_string(m_slice_dimension);
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->slices = std::move(m_slices);

        QDialog::done(r);
}

std::optional<ImageSliceParameters> ImageSliceDialog::show(const std::vector<int>& size, int slice_dimension)
{
        std::optional<ImageSliceParameters> parameters;

        QtObjectInDynamicMemory w(new ImageSliceDialog(size, slice_dimension, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
