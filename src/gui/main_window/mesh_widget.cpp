/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "model_tree.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/gui/com/support.h>
#include <src/gui/dialogs/color_dialog.h>
#include <src/model/mesh_object.h>
#include <src/model/object_id.h>
#include <src/storage/types.h>

#include <QColor>
#include <QLabel>
#include <QPointer>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>
#include <QToolButton>
#include <QWidget>

#include <cstddef>
#include <iomanip>
#include <ios>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <variant>

namespace ns::gui::main_window
{
namespace
{
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

struct MeshInfo final
{
        double alpha;
        color::Color color;
        double ambient;
        double metalness;
        double roughness;
};

template <std::size_t N>
MeshInfo read_mesh(const model::mesh::MeshObject<N>& mesh_object)
{
        const model::mesh::Reading reading(mesh_object);

        return {.alpha = reading.alpha(),
                .color = reading.color(),
                .ambient = reading.ambient(),
                .metalness = reading.metalness(),
                .roughness = reading.roughness()};
}
}

MeshWidget::MeshWidget()
        : QWidget(nullptr)
{
        ui_.setupUi(this);

        widgets_.reserve(this->findChildren<QWidget*>().size());
        for (QWidget* const widget : this->findChildren<QWidget*>())
        {
                widgets_.emplace_back(widget);
        }

        set_model_tree(nullptr);

        connect(ui_.slider_ambient, &QSlider::valueChanged, this, &MeshWidget::on_ambient_changed);
        connect(ui_.slider_metalness, &QSlider::valueChanged, this, &MeshWidget::on_metalness_changed);
        connect(ui_.slider_roughness, &QSlider::valueChanged, this, &MeshWidget::on_roughness_changed);
        connect(ui_.slider_transparency, &QSlider::valueChanged, this, &MeshWidget::on_transparency_changed);
        connect(ui_.tool_button_color, &QToolButton::clicked, this, &MeshWidget::on_color_clicked);

        this->adjustSize();

        const auto h = ui_.tool_button_color->size().height();
        ui_.widget_color->setMinimumSize(h, h);
}

void MeshWidget::set_model_tree(ModelTree* const model_tree)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        model_tree_ = model_tree;
        if (model_tree)
        {
                connect(model_tree_, &ModelTree::item_update, this, &MeshWidget::on_model_tree_item_update);
                on_model_tree_item_update();
        }
        else
        {
                ui_disable();
        }
}

void MeshWidget::set_enabled(const bool enabled) const
{
        for (const QPointer<QWidget>& widget : widgets_)
        {
                ASSERT(widget);
                widget->setEnabled(enabled);
        }
}

void MeshWidget::on_ambient_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> object_opt = model_tree_->current_mesh();
        if (!object_opt)
        {
                return;
        }

        const double ambient = slider_position(ui_.slider_ambient);

        set_label(ui_.label_ambient, ambient);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                {
                        model::mesh::Writing writing(object.get());
                        writing.set_ambient(ambient);
                },
                *object_opt);
}

void MeshWidget::on_metalness_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> object_opt = model_tree_->current_mesh();
        if (!object_opt)
        {
                return;
        }

        const double metalness = slider_position(ui_.slider_metalness);

        set_label(ui_.label_metalness, metalness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                {
                        model::mesh::Writing writing(object.get());
                        writing.set_metalness(metalness);
                },
                *object_opt);
}

void MeshWidget::on_roughness_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> object_opt = model_tree_->current_mesh();
        if (!object_opt)
        {
                return;
        }

        const double roughness = slider_position(ui_.slider_roughness);

        set_label(ui_.label_roughness, roughness);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                {
                        model::mesh::Writing writing(object.get());
                        writing.set_roughness(roughness);
                },
                *object_opt);
}

void MeshWidget::on_transparency_changed()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> object_opt = model_tree_->current_mesh();
        if (!object_opt)
        {
                return;
        }

        const double alpha = 1.0 - slider_position(ui_.slider_transparency);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                {
                        model::mesh::Writing writing(object.get());
                        writing.set_alpha(alpha);
                },
                *object_opt);
}

void MeshWidget::on_color_clicked()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> object_opt = model_tree_->current_mesh();
        if (!object_opt)
        {
                return;
        }

        color::Color color;
        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                {
                        const model::mesh::Reading reading(*object);
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
                                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
                                {
                                        set_widget_color(ui_.widget_color, c);
                                        model::mesh::Writing writing(object.get());
                                        writing.set_color(qcolor_to_color(c));
                                },
                                *object_opt);
                });
}

void MeshWidget::on_model_tree_item_update()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<model::ObjectId> id = model_tree_->current_item();
        if (!id)
        {
                ui_disable();
                return;
        }

        const std::optional<storage::MeshObjectConst> mesh = model_tree_->mesh_const_if_current(*id);
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
        ASSERT(std::this_thread::get_id() == thread_id_);

        set_enabled(false);

        {
                const QSignalBlocker blocker(ui_.widget_color);
                set_widget_color(ui_.widget_color, QColor(255, 255, 255));
        }
        {
                const QSignalBlocker blocker(ui_.slider_transparency);
                set_slider_position(ui_.slider_transparency, 0);
        }
        {
                const QSignalBlocker blocker(ui_.slider_ambient);
                set_slider_to_middle(ui_.slider_ambient);
                ui_.label_ambient->clear();
        }
        {
                const QSignalBlocker blocker(ui_.slider_metalness);
                set_slider_to_middle(ui_.slider_metalness);
                ui_.label_metalness->clear();
        }
        {
                const QSignalBlocker blocker(ui_.slider_roughness);
                set_slider_to_middle(ui_.slider_roughness);
                ui_.label_roughness->clear();
        }
}

void MeshWidget::ui_set(const storage::MeshObjectConst& object)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        set_enabled(true);

        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const model::mesh::MeshObject<N>>& mesh_object)
                {
                        const MeshInfo info = read_mesh(*mesh_object);

                        {
                                const double position = 1.0 - info.alpha;
                                const QSignalBlocker blocker(ui_.slider_transparency);
                                set_slider_position(ui_.slider_transparency, position);
                        }
                        {
                                const QSignalBlocker blocker(ui_.widget_color);
                                set_widget_color(ui_.widget_color, color_to_qcolor(info.color));
                        }
                        {
                                const double position = info.ambient;
                                const QSignalBlocker blocker(ui_.slider_ambient);
                                set_slider_position(ui_.slider_ambient, position);
                                set_label(ui_.label_ambient, ui_.slider_ambient);
                        }
                        {
                                const double position = info.metalness;
                                const QSignalBlocker blocker(ui_.slider_metalness);
                                set_slider_position(ui_.slider_metalness, position);
                                set_label(ui_.label_metalness, ui_.slider_metalness);
                        }
                        {
                                const double position = info.roughness;
                                const QSignalBlocker blocker(ui_.slider_roughness);
                                set_slider_position(ui_.slider_roughness, position);
                                set_label(ui_.label_roughness, ui_.slider_roughness);
                        }
                },
                object);
}
}
