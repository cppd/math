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

#include "mesh_widget.h"

#include "../com/support.h"
#include "../dialogs/color_dialog.h"

#include <src/com/error.h>

namespace ns::gui::main_window
{
MeshWidget::MeshWidget(double maximum_specular_power)
        : QWidget(nullptr), m_maximum_specular_power(maximum_specular_power)
{
        ui.setupUi(this);

        m_widgets.reserve(this->findChildren<QWidget*>().size());
        for (QWidget* widget : this->findChildren<QWidget*>())
        {
                m_widgets.emplace_back(widget);
        }

        set_model_tree(nullptr);

        connect(ui.slider_ambient, &QSlider::valueChanged, this, &MeshWidget::on_ambient_changed);
        connect(ui.slider_diffuse, &QSlider::valueChanged, this, &MeshWidget::on_diffuse_changed);
        connect(ui.slider_specular_power, &QSlider::valueChanged, this, &MeshWidget::on_specular_power_changed);
        connect(ui.slider_specular, &QSlider::valueChanged, this, &MeshWidget::on_specular_changed);
        connect(ui.slider_transparency, &QSlider::valueChanged, this, &MeshWidget::on_transparency_changed);
        connect(ui.toolButton_color, &QToolButton::clicked, this, &MeshWidget::on_color_clicked);
}

void MeshWidget::set_model_tree(ModelTree* model_tree)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_model_tree = model_tree;
        if (model_tree)
        {
                connect(m_model_tree, &ModelTree::item_update, this, &MeshWidget::on_model_tree_item_update);
                on_model_tree_item_update();
        }
        else
        {
                ui_disable();
        }
}

void MeshWidget::set_enabled(bool enabled) const
{
        for (const QPointer<QWidget>& widget : m_widgets)
        {
                ASSERT(widget);
                widget->setEnabled(enabled);
        }
}

void MeshWidget::on_ambient_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double ambient = slider_position(ui.slider_ambient);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void MeshWidget::on_diffuse_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double diffuse = slider_position(ui.slider_diffuse);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_diffuse(diffuse);
                },
                *object_opt);
}

void MeshWidget::on_specular_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double specular = slider_position(ui.slider_specular);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_specular(specular);
                },
                *object_opt);
}

void MeshWidget::on_specular_power_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double specular_power = std::pow(m_maximum_specular_power, slider_position(ui.slider_specular_power));

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_specular_power(specular_power);
                },
                *object_opt);
}

void MeshWidget::on_transparency_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double alpha = 1.0 - slider_position(ui.slider_transparency);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_alpha(alpha);
                },
                *object_opt);
}

void MeshWidget::on_color_clicked()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        Color color;
        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Reading reading(*object);
                        color = reading.color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog(
                "Mesh Color", rgb_to_qcolor(color),
                [&](const QColor& c)
                {
                        if (ptr.isNull())
                        {
                                return;
                        }
                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                                {
                                        set_widget_color(ui.widget_color, c);
                                        mesh::Writing writing(object.get());
                                        writing.set_color(qcolor_to_rgb(c));
                                },
                                *object_opt);
                });
}

void MeshWidget::on_model_tree_item_update()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = m_model_tree->current_item();
        if (!id)
        {
                ui_disable();
                return;
        }

        std::optional<storage::MeshObjectConst> mesh = m_model_tree->mesh_const_if_current(*id);
        if (mesh)
        {
                ui_set(*mesh);
        }
        else
        {
                ui_disable();
        }
}

void MeshWidget::ui_disable()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        set_enabled(false);

        {
                QSignalBlocker blocker(ui.widget_color);
                set_widget_color(ui.widget_color, QColor(255, 255, 255));
        }
        {
                QSignalBlocker blocker(ui.slider_transparency);
                set_slider_position(ui.slider_transparency, 0);
        }
        {
                QSignalBlocker blocker(ui.slider_ambient);
                set_slider_to_middle(ui.slider_ambient);
        }
        {
                QSignalBlocker blocker(ui.slider_diffuse);
                set_slider_to_middle(ui.slider_diffuse);
        }
        {
                QSignalBlocker blocker(ui.slider_specular);
                set_slider_to_middle(ui.slider_specular);
        }
        {
                QSignalBlocker blocker(ui.slider_specular_power);
                set_slider_to_middle(ui.slider_specular_power);
        }
}

void MeshWidget::ui_set(const storage::MeshObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        set_enabled(true);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const mesh::MeshObject<N>>& mesh_object)
                {
                        double alpha;
                        Color color;
                        double ambient;
                        double diffuse;
                        double specular;
                        double specular_power;
                        {
                                mesh::Reading reading(*mesh_object);
                                alpha = reading.alpha();
                                color = reading.color();
                                ambient = reading.ambient();
                                diffuse = reading.diffuse();
                                specular = reading.specular();
                                specular_power = reading.specular_power();
                        }
                        {
                                double position = 1.0 - alpha;
                                QSignalBlocker blocker(ui.slider_transparency);
                                set_slider_position(ui.slider_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.widget_color);
                                set_widget_color(ui.widget_color, color);
                        }
                        {
                                double position = ambient;
                                QSignalBlocker blocker(ui.slider_ambient);
                                set_slider_position(ui.slider_ambient, position);
                        }
                        {
                                double position = diffuse;
                                QSignalBlocker blocker(ui.slider_diffuse);
                                set_slider_position(ui.slider_diffuse, position);
                        }
                        {
                                double position = specular;
                                QSignalBlocker blocker(ui.slider_specular);
                                set_slider_position(ui.slider_specular, position);
                        }
                        {
                                double position = std::log(std::clamp(specular_power, 1.0, m_maximum_specular_power))
                                                  / std::log(m_maximum_specular_power);
                                QSignalBlocker blocker(ui.slider_specular_power);
                                set_slider_position(ui.slider_specular_power, position);
                        }
                },
                object);
}
}
