/*
Copyright (C) 2017-2020 Topological Manifold

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

namespace gui
{
namespace
{
// Максимальный коэффициент для умножения и деления α на него.
constexpr double VOLUME_ALPHA_COEFFICIENT = 20;
}

VolumeWidget::VolumeWidget(ModelTree* model_tree, double maximum_specular_power, double maximum_model_lighting)
        : QWidget(nullptr),
          m_maximum_specular_power(maximum_specular_power),
          m_maximum_model_lighting(maximum_model_lighting),
          m_model_tree(model_tree)
{
        ASSERT(m_model_tree);

        ui.setupUi(this);

        m_widgets.reserve(this->findChildren<QWidget*>().size());
        for (QWidget* widget : this->findChildren<QWidget*>())
        {
                m_widgets.emplace_back(widget);
        }

        m_slider_volume_levels = std::make_unique<RangeSlider>(ui.slider_volume_level_min, ui.slider_volume_level_max);

        volume_ui_disable();

        connect(ui.checkBox_isosurface, &QCheckBox::clicked, this, &VolumeWidget::on_isosurface_clicked);
        connect(ui.slider_isosurface_transparency, &QSlider::valueChanged, this,
                &VolumeWidget::on_isosurface_transparency_changed);
        connect(ui.slider_isovalue, &QSlider::valueChanged, this, &VolumeWidget::on_isovalue_changed);
        connect(ui.slider_volume_ambient, &QSlider::valueChanged, this, &VolumeWidget::on_volume_ambient_changed);
        connect(ui.slider_volume_diffuse, &QSlider::valueChanged, this, &VolumeWidget::on_volume_diffuse_changed);
        connect(ui.slider_volume_specular_power, &QSlider::valueChanged, this,
                &VolumeWidget::on_volume_specular_power_changed);
        connect(ui.slider_volume_specular, &QSlider::valueChanged, this, &VolumeWidget::on_volume_specular_changed);
        connect(ui.slider_volume_transparency, &QSlider::valueChanged, this,
                &VolumeWidget::on_volume_transparency_changed);
        connect(ui.toolButton_volume_color, &QToolButton::clicked, this, &VolumeWidget::on_volume_color_clicked);
        connect(m_slider_volume_levels.get(), &RangeSlider::changed, this, &VolumeWidget::on_volume_levels_changed);
}

void VolumeWidget::set_enabled(bool enabled) const
{
        for (const QPointer<QWidget>& widget : m_widgets)
        {
                ASSERT(widget);
                widget->setEnabled(enabled);
        }
}

void VolumeWidget::on_volume_levels_changed(double min, double max)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_levels(min, max);
                },
                *volume_object_opt);
}

void VolumeWidget::on_volume_transparency_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        double log_alpha_coefficient = 1.0 - 2.0 * slider_position(ui.slider_volume_transparency);
        double alpha_coefficient = std::pow(VOLUME_ALPHA_COEFFICIENT, log_alpha_coefficient);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_volume_alpha_coefficient(alpha_coefficient);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isosurface_transparency_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        double alpha = 1.0 - slider_position(ui.slider_isosurface_transparency);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface_alpha(alpha);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isosurface_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        bool checked = ui.checkBox_isosurface->isChecked();
        ui.slider_isovalue->setEnabled(checked);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_isosurface(checked);
                },
                *volume_object_opt);
}

void VolumeWidget::on_isovalue_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> volume_object_opt = m_model_tree->current_volume();
        if (!volume_object_opt)
        {
                return;
        }

        float isovalue = slider_position(ui.slider_isovalue);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                        volume::Writing writing(volume_object.get());
                        writing.set_isovalue(isovalue);
                },
                *volume_object_opt);
}

void VolumeWidget::on_volume_color_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        Color color;
        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Reading reading(*object);
                        color = reading.color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog("Volume Color", rgb_to_qcolor(color), [&](const QColor& c) {
                if (ptr.isNull())
                {
                        return;
                }
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                                set_widget_color(ui.widget_volume_color, c);
                                volume::Writing writing(object.get());
                                writing.set_color(qcolor_to_rgb(c));
                        },
                        *object_opt);
        });
}

void VolumeWidget::on_volume_ambient_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double ambient = m_maximum_model_lighting * slider_position(ui.slider_volume_ambient);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void VolumeWidget::on_volume_diffuse_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double diffuse = m_maximum_model_lighting * slider_position(ui.slider_volume_diffuse);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_diffuse(diffuse);
                },
                *object_opt);
}

void VolumeWidget::on_volume_specular_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double specular = m_maximum_model_lighting * slider_position(ui.slider_volume_specular);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_specular(specular);
                },
                *object_opt);
}

void VolumeWidget::on_volume_specular_power_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::VolumeObject> object_opt = m_model_tree->current_volume();
        if (!object_opt)
        {
                return;
        }

        double specular_power = std::pow(m_maximum_specular_power, slider_position(ui.slider_volume_specular_power));

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                        volume::Writing writing(object.get());
                        writing.set_specular_power(specular_power);
                },
                *object_opt);
}

void VolumeWidget::volume_ui_disable()
{
        set_enabled(false);

        {
                QSignalBlocker blocker(m_slider_volume_levels.get());
                m_slider_volume_levels->set_range(0, 1);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_transparency);
                set_slider_to_middle(ui.slider_volume_transparency);
        }
        {
                QSignalBlocker blocker(ui.checkBox_isosurface);
                ui.checkBox_isosurface->setChecked(false);
        }
        {
                QSignalBlocker blocker(ui.slider_isovalue);
                set_slider_to_middle(ui.slider_isovalue);
        }
        {
                QSignalBlocker blocker(ui.slider_isosurface_transparency);
                set_slider_position(ui.slider_isosurface_transparency, 0);
        }
        {
                QSignalBlocker blocker(ui.widget_volume_color);
                set_widget_color(ui.widget_volume_color, QColor(255, 255, 255));
        }
        {
                QSignalBlocker blocker(ui.slider_volume_ambient);
                set_slider_to_middle(ui.slider_volume_ambient);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_diffuse);
                set_slider_to_middle(ui.slider_volume_diffuse);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_specular);
                set_slider_to_middle(ui.slider_volume_specular);
        }
        {
                QSignalBlocker blocker(ui.slider_volume_specular_power);
                set_slider_to_middle(ui.slider_volume_specular_power);
        }
}

void VolumeWidget::volume_ui_set(const storage::VolumeObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        set_enabled(true);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<const volume::VolumeObject<N>>& volume_object) {
                        double min;
                        double max;
                        double volume_alpha_coefficient;
                        double isosurface_alpha;
                        bool isosurface;
                        float isovalue;
                        Color color;
                        double ambient, diffuse, specular, specular_power;
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
                                diffuse = reading.diffuse();
                                specular = reading.specular();
                                specular_power = reading.specular_power();
                        }
                        {
                                QSignalBlocker blocker(m_slider_volume_levels.get());
                                m_slider_volume_levels->set_range(min, max);
                        }
                        {
                                volume_alpha_coefficient = std::clamp(
                                        volume_alpha_coefficient, 1.0 / VOLUME_ALPHA_COEFFICIENT,
                                        VOLUME_ALPHA_COEFFICIENT);
                                double log_volume_alpha_coefficient =
                                        std::log(volume_alpha_coefficient) / std::log(VOLUME_ALPHA_COEFFICIENT);
                                double position = 0.5 * (1.0 - log_volume_alpha_coefficient);
                                QSignalBlocker blocker(ui.slider_volume_transparency);
                                ui.slider_volume_transparency->setEnabled(!isosurface);
                                set_slider_position(ui.slider_volume_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.checkBox_isosurface);
                                ui.checkBox_isosurface->setChecked(isosurface);
                        }
                        {
                                double position = 1.0 - isosurface_alpha;
                                QSignalBlocker blocker(ui.slider_isosurface_transparency);
                                ui.slider_isosurface_transparency->setEnabled(isosurface);
                                set_slider_position(ui.slider_isosurface_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.slider_isovalue);
                                ui.slider_isovalue->setEnabled(isosurface);
                                set_slider_position(ui.slider_isovalue, isovalue);
                        }
                        {
                                QSignalBlocker blocker(ui.widget_volume_color);
                                set_widget_color(ui.widget_volume_color, color);
                        }
                        {
                                double position = ambient / m_maximum_model_lighting;
                                QSignalBlocker blocker(ui.slider_volume_ambient);
                                set_slider_position(ui.slider_volume_ambient, position);
                        }
                        {
                                double position = diffuse / m_maximum_model_lighting;
                                QSignalBlocker blocker(ui.slider_volume_diffuse);
                                set_slider_position(ui.slider_volume_diffuse, position);
                        }
                        {
                                double position = specular / m_maximum_model_lighting;
                                QSignalBlocker blocker(ui.slider_volume_specular);
                                set_slider_position(ui.slider_volume_specular, position);
                        }
                        {
                                double position = std::log(std::clamp(specular_power, 1.0, m_maximum_specular_power))
                                                  / std::log(m_maximum_specular_power);
                                QSignalBlocker blocker(ui.slider_volume_specular_power);
                                set_slider_position(ui.slider_volume_specular_power, position);
                        }
                },
                object);
}

void VolumeWidget::update()
{
        std::optional<ObjectId> id = m_model_tree->current_item();
        if (!id)
        {
                volume_ui_disable();
                return;
        }

        std::optional<storage::VolumeObjectConst> volume = m_model_tree->volume_const_if_current(*id);
        if (volume)
        {
                volume_ui_set(*volume);
        }
        else
        {
                volume_ui_disable();
        }
}
}
