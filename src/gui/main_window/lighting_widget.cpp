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

#include "lighting_widget.h"

#include "../com/support.h"

#include <src/color/illuminants.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ns::gui::main_window
{
namespace
{
constexpr double DEFAULT_LIGHTING_INTENSITY = 2.0;
constexpr double MAXIMUM_LIGHTING_INTENSITY = 20.0;
static_assert(MAXIMUM_LIGHTING_INTENSITY > 1);

double position_to_intensity(double position)
{
        const double v = 2 * position - 1;
        const double intensity = std::pow(MAXIMUM_LIGHTING_INTENSITY, v);
        return std::clamp(intensity, 1 / MAXIMUM_LIGHTING_INTENSITY, MAXIMUM_LIGHTING_INTENSITY);
}

double intensity_to_position(double intensity)
{
        const double position = std::log(intensity) / std::log(MAXIMUM_LIGHTING_INTENSITY);
        return std::clamp((position + 1) / 2, 0.0, 1.0);
}
}

LightingWidget::LightingWidget() : QWidget(nullptr)
{
        ui.setupUi(this);

        m_spectrum = color::daylight_d65();
        m_rgb = color::Color(1, 1, 1);

        ui.slider_intensity->setMinimum(0);
        ui.slider_intensity->setMaximum(1000);
        connect(ui.slider_intensity, &QSlider::valueChanged, this, &LightingWidget::on_intensity_changed);
        set_slider_position(ui.slider_intensity, intensity_to_position(DEFAULT_LIGHTING_INTENSITY));
}

void LightingWidget::set_view(view::View* view)
{
        m_view = view;
}

void LightingWidget::on_intensity_changed()
{
        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << intensity();
        ui.label_intensity->setText(QString::fromStdString(oss.str()));

        if (m_view)
        {
                m_view->send(view::command::SetLightingColor(rgb()));
        }
}

double LightingWidget::intensity() const
{
        return position_to_intensity(slider_position(ui.slider_intensity));
}

color::Spectrum LightingWidget::color_spectrum() const
{
        return m_spectrum;
}

color::Color LightingWidget::color_rgb() const
{
        return m_rgb;
}

color::Spectrum LightingWidget::spectrum() const
{
        return intensity() * color_spectrum();
}

color::Color LightingWidget::rgb() const
{
        return intensity() * color_rgb();
}

std::tuple<color::Spectrum, color::Color> LightingWidget::color() const
{
        const double v = intensity();
        return {v * color_spectrum(), v * color_rgb()};
}
}
