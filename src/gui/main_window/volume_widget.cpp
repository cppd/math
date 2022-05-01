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

#include "volume_widget.h"

#include "../com/support.h"
#include "../dialogs/color_dialog.h"

#include <iomanip>
#include <sstream>

namespace ns::gui::main_window
{
namespace
{
// [1/C, C]
constexpr double VOLUME_ALPHA_COEFFICIENT = 250;

void set_label(QLabel* const label, const double value)
{
        std::ostringstream oss;
        oss << std::setprecision(3) << std::fixed << value;
        label->setText(QString::fromStdString(oss.str()));
}

void set_label(QLabel* const label, QSlider* const slider)
{
        set_label(label, slider_position(slider));
}
}

VolumeWidget::VolumeWidget() : QWidget(nullptr)
{
        ui_.setupUi(this);

        widgets_.reserve(this->findChildren<QWidget*>().size());
        for (QWidget* widget : this->findChildren<QWidget*>())
        {
                widgets_.emplace_back(widget);
        }

        slider_levels_ = std::make_unique<RangeSlider>(ui_.slider_level_min, ui_.slider_level_max);

        set_model_tree(nullptr);

        connect(ui_.checkBox_isosurface, &QCheckBox::clicked, this, &VolumeWidget::on_isosurface_clicked);
        connect(ui_.slider_isosurface_transparency, &QSlider::valueChanged, this,
                &VolumeWidget::on_isosurface_transparency_changed);
        connect(ui_.slider_isovalue, &QSlider::valueChanged, this, &VolumeWidget::on_isovalue_changed);
        connect(ui_.slider_ambient, &QSlider::valueChanged, this, &VolumeWidget::on_ambient_changed);
        connect(ui_.slider_metalness, &QSlider::valueChanged, this, &VolumeWidget::on_metalness_changed);
        connect(ui_.slider_roughness, &QSlider::valueChanged, this, &VolumeWidget::on_roughness_changed);
        connect(ui_.slider_transparency, &QSlider::valueChanged, this, &VolumeWidget::on_transparency_changed);
        connect(ui_.toolButton_color, &QToolButton::clicked, this, &VolumeWidget::on_color_clicked);
        connect(slider_levels_.get(), &RangeSlider::changed, this, &VolumeWidget::on_levels_changed);

        this->adjustSize();

        const auto h = ui_.toolButton_color->size().height();
        ui_.widget_color->setMinimumSize(h, h);
}

void VolumeWidget::set_model_tree(ModelTree* const model_tree)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        model_tree_ = model_tree;
        if (model_tree)
        {
                connect(model_tree_, &ModelTree::item_update, this, &VolumeWidget::on_model_tree_item_update);
                on_model_tree_item_update();
        }
        else
        {
                ui_disable();
        }
}

void VolumeWidget::set_enabled(const bool enabled) const
{
        for (const QPointer<QWidget>& widget : widgets_)
        {
                ASSERT(widget);
                widget->setEnabled(enabled);
        }
}

void VolumeWidget::on_levels_changed(const double min, const double max)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> volume_object_opt = model_tree_->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                {
                        volume::Writing writing(volume_object.get());
                        writing.set_levels(min, max);
                },
                *volume_object_opt);
}

void VolumeWidget::on_transparency_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> volume_object_opt = model_tree_->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        const double log_alpha_coefficient = 1.0 - 2.0 * slider_position(ui_.slider_transparency);
        const double alpha_coefficient = std::pow(VOLUME_ALPHA_COEFFICIENT, log_alpha_coefficient);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                {
                        volume::Writing writing(volume_object.get());
                        writing.set_volume_alpha_coefficient(alpha_coefficient);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isosurface_transparency_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> volume_object_opt = model_tree_->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        const double alpha = 1.0 - slider_position(ui_.slider_isosurface_transparency);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                {
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface_alpha(alpha);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isosurface_clicked()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const bool checked = ui_.checkBox_isosurface->isChecked();
        ui_.slider_isovalue->setEnabled(checked);

        const std::optional<storage::VolumeObject> volume_object_opt = model_tree_->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                {
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface(checked);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isovalue_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> volume_object_opt = model_tree_->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        const float isovalue = slider_position(ui_.slider_isovalue);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                {
                        volume::Writing writing(volume_object.get());
                        writing.set_isovalue(isovalue);
                },
                *volume_object_opt);
}

void VolumeWidget::on_color_clicked()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> object_opt = model_tree_->current_volume();
        if (!object_opt)
        {
                return;
        }

        color::Color color;
        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                {
                        volume::Reading reading(*object);
                        color = reading.color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog(
                "Volume Color", color_to_qcolor(color),
                [&](const QColor& c)
                {
                        if (ptr.isNull())
                        {
                                return;
                        }
                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                                {
                                        set_widget_color(ui_.widget_color, c);
                                        volume::Writing writing(object.get());
                                        writing.set_color(qcolor_to_color(c));
                                },
                                *object_opt);
                });
}

void VolumeWidget::on_ambient_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> object_opt = model_tree_->current_volume();
        if (!object_opt)
        {
                return;
        }

        const double ambient = slider_position(ui_.slider_ambient);

        set_label(ui_.label_ambient, ambient);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                {
                        volume::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void VolumeWidget::on_metalness_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> object_opt = model_tree_->current_volume();
        if (!object_opt)
        {
                return;
        }

        const double metalness = slider_position(ui_.slider_metalness);

        set_label(ui_.label_metalness, metalness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                {
                        volume::Writing writing(object.get());
                        writing.set_metalness(metalness);
                },
                *object_opt);
}

void VolumeWidget::on_roughness_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::VolumeObject> object_opt = model_tree_->current_volume();
        if (!object_opt)
        {
                return;
        }

        const double roughness = slider_position(ui_.slider_roughness);

        set_label(ui_.label_roughness, roughness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                {
                        volume::Writing writing(object.get());
                        writing.set_roughness(roughness);
                },
                *object_opt);
}

void VolumeWidget::on_model_tree_item_update()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<ObjectId> id = model_tree_->current_item();
        if (!id)
        {
                ui_disable();
                return;
        }

        const std::optional<storage::VolumeObjectConst> volume = model_tree_->volume_const_if_current(*id);
        if (volume)
        {
                ui_set(*volume);
        }
        else
        {
                ui_disable();
        }
}

void VolumeWidget::ui_disable()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        set_enabled(false);

        {
                QSignalBlocker blocker(slider_levels_.get());
                slider_levels_->set_range(0, 1);
        }
        {
                QSignalBlocker blocker(ui_.slider_transparency);
                set_slider_to_middle(ui_.slider_transparency);
        }
        {
                QSignalBlocker blocker(ui_.checkBox_isosurface);
                ui_.checkBox_isosurface->setChecked(false);
        }
        {
                QSignalBlocker blocker(ui_.slider_isovalue);
                set_slider_to_middle(ui_.slider_isovalue);
        }
        {
                QSignalBlocker blocker(ui_.slider_isosurface_transparency);
                set_slider_position(ui_.slider_isosurface_transparency, 0);
        }
        {
                QSignalBlocker blocker(ui_.widget_color);
                set_widget_color(ui_.widget_color, QColor(255, 255, 255));
        }
        {
                QSignalBlocker blocker(ui_.slider_ambient);
                set_slider_to_middle(ui_.slider_ambient);
                ui_.label_ambient->clear();
        }
        {
                QSignalBlocker blocker(ui_.slider_metalness);
                set_slider_to_middle(ui_.slider_metalness);
                ui_.label_metalness->clear();
        }
        {
                QSignalBlocker blocker(ui_.slider_roughness);
                set_slider_to_middle(ui_.slider_roughness);
                ui_.label_roughness->clear();
        }
}

void VolumeWidget::ui_set(const storage::VolumeObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        set_enabled(true);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const volume::VolumeObject<N>>& volume_object)
                {
                        double min;
                        double max;
                        double volume_alpha_coefficient;
                        double isosurface_alpha;
                        bool isosurface;
                        float isovalue;
                        color::Color color;
                        double ambient;
                        double metalness;
                        double roughness;
                        {
                                volume::Reading reading(*volume_object);
                                min = reading.level_min();
                                max = reading.level_max();
                                volume_alpha_coefficient = reading.volume_alpha_coefficient();
                                isosurface_alpha = reading.isosurface_alpha();
                                isosurface = reading.isosurface();
                                isovalue = reading.isovalue();
                                color = reading.color();
                                ambient = reading.ambient();
                                metalness = reading.metalness();
                                roughness = reading.roughness();
                        }
                        {
                                QSignalBlocker blocker(slider_levels_.get());
                                slider_levels_->set_range(min, max);
                        }
                        {
                                volume_alpha_coefficient = std::clamp(
                                        volume_alpha_coefficient, 1.0 / VOLUME_ALPHA_COEFFICIENT,
                                        VOLUME_ALPHA_COEFFICIENT);
                                const double log_volume_alpha_coefficient =
                                        std::log(volume_alpha_coefficient) / std::log(VOLUME_ALPHA_COEFFICIENT);
                                const double position = 0.5 * (1.0 - log_volume_alpha_coefficient);
                                QSignalBlocker blocker(ui_.slider_transparency);
                                ui_.slider_transparency->setEnabled(!isosurface);
                                set_slider_position(ui_.slider_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui_.checkBox_isosurface);
                                ui_.checkBox_isosurface->setChecked(isosurface);
                        }
                        {
                                const double position = 1.0 - isosurface_alpha;
                                QSignalBlocker blocker(ui_.slider_isosurface_transparency);
                                ui_.slider_isosurface_transparency->setEnabled(isosurface);
                                set_slider_position(ui_.slider_isosurface_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui_.slider_isovalue);
                                ui_.slider_isovalue->setEnabled(isosurface);
                                set_slider_position(ui_.slider_isovalue, isovalue);
                        }
                        {
                                QSignalBlocker blocker(ui_.widget_color);
                                set_widget_color(ui_.widget_color, color_to_qcolor(color));
                        }
                        {
                                const double position = ambient;
                                QSignalBlocker blocker(ui_.slider_ambient);
                                set_slider_position(ui_.slider_ambient, position);
                                set_label(ui_.label_ambient, ui_.slider_ambient);
                        }
                        {
                                const double position = metalness;
                                QSignalBlocker blocker(ui_.slider_metalness);
                                set_slider_position(ui_.slider_metalness, position);
                                set_label(ui_.label_metalness, ui_.slider_metalness);
                        }
                        {
                                const double position = roughness;
                                QSignalBlocker blocker(ui_.slider_roughness);
                                set_slider_position(ui_.slider_roughness, position);
                                set_label(ui_.label_roughness, ui_.slider_roughness);
                        }
                },
                object);
}
}
