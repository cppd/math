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
MeshWidget::MeshWidget() : QWidget(nullptr)
{
        ui.setupUi(this);

        m_widgets.reserve(this->findChildren<QWidget*>().size());
        for (QWidget* widget : this->findChildren<QWidget*>())
        {
                m_widgets.emplace_back(widget);
        }

        set_model_tree(nullptr);

        connect(ui.slider_ambient, &QSlider::valueChanged, this, &MeshWidget::on_ambient_changed);
        connect(ui.slider_metalness, &QSlider::valueChanged, this, &MeshWidget::on_metalness_changed);
        connect(ui.slider_roughness, &QSlider::valueChanged, this, &MeshWidget::on_roughness_changed);
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

void MeshWidget::on_metalness_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double metalness = slider_position(ui.slider_metalness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_metalness(metalness);
                },
                *object_opt);
}

void MeshWidget::on_roughness_changed(int)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> object_opt = m_model_tree->current_mesh();
        if (!object_opt)
        {
                return;
        }

        double roughness = slider_position(ui.slider_roughness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Writing writing(object.get());
                        writing.set_roughness(roughness);
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

        color::Color color;
        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                {
                        mesh::Reading reading(*object);
                        color = reading.color();
                },
                *object_opt);

        QPointer ptr(this);
        dialog::color_dialog(
                "Mesh Color", color_to_qcolor(color),
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
                                        writing.set_color(qcolor_to_color(c));
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
                QSignalBlocker blocker(ui.slider_metalness);
                set_slider_to_middle(ui.slider_metalness);
        }
        {
                QSignalBlocker blocker(ui.slider_roughness);
                set_slider_to_middle(ui.slider_roughness);
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
                        color::Color color;
                        double ambient;
                        double metalness;
                        double roughness;
                        {
                                mesh::Reading reading(*mesh_object);
                                alpha = reading.alpha();
                                color = reading.color();
                                ambient = reading.ambient();
                                metalness = reading.metalness();
                                roughness = reading.roughness();
                        }
                        {
                                double position = 1.0 - alpha;
                                QSignalBlocker blocker(ui.slider_transparency);
                                set_slider_position(ui.slider_transparency, position);
                        }
                        {
                                QSignalBlocker blocker(ui.widget_color);
                                set_widget_color(ui.widget_color, color_to_qcolor(color));
                        }
                        {
                                double position = ambient;
                                QSignalBlocker blocker(ui.slider_ambient);
                                set_slider_position(ui.slider_ambient, position);
                        }
                        {
                                double position = metalness;
                                QSignalBlocker blocker(ui.slider_metalness);
                                set_slider_position(ui.slider_metalness, position);
                        }
                        {
                                double position = roughness;
                                QSignalBlocker blocker(ui.slider_roughness);
                                set_slider_position(ui.slider_roughness, position);
                        }
                },
                object);
}
}
