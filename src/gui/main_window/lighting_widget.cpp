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
#include <src/com/error.h>
#include <src/com/print.h>

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

constexpr int DAYLIGHT_CCT_INTERVAL = 100;
constexpr std::string_view DAYLIGHT_D65 = "Daylight D65";
constexpr std::string_view DAYLIGHT_CCT = "Daylight %1K";

double ceil(double v, int t)
{
        return std::ceil(v / t) * t;
}

double floor(double v, int t)
{
        return std::floor(v / t) * t;
}

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

LightingWidget::LightingWidget()
        : QWidget(nullptr),
          m_daylight_min_cct(ceil(color::daylight_min_cct(), DAYLIGHT_CCT_INTERVAL)),
          m_daylight_max_cct(floor(color::daylight_max_cct(), DAYLIGHT_CCT_INTERVAL))
{
        if (!(m_daylight_max_cct > m_daylight_min_cct))
        {
                error("Error daylight CCT min " + to_string(m_daylight_min_cct) + " and max "
                      + to_string(m_daylight_max_cct));
        }

        ui.setupUi(this);

        ui.slider_intensity->setMinimum(0);
        ui.slider_intensity->setMaximum(1000);
        set_slider_position(ui.slider_intensity, intensity_to_position(DEFAULT_LIGHTING_INTENSITY));

        ASSERT((m_daylight_max_cct - m_daylight_min_cct) % DAYLIGHT_CCT_INTERVAL == 0);
        ui.slider_daylight_cct->setMinimum(0);
        ui.slider_daylight_cct->setMaximum((m_daylight_max_cct - m_daylight_min_cct) / DAYLIGHT_CCT_INTERVAL);
        set_slider_to_middle(ui.slider_daylight_cct);

        ui.radioButton_d65->setChecked(true);
        ui.radioButton_d65->setText(QString::fromUtf8(DAYLIGHT_D65.data(), DAYLIGHT_D65.size()));

        on_intensity_changed();
        on_daylight_changed();
        on_d65_toggled();
        on_daylight_toggled();

        connect(ui.slider_intensity, &QSlider::valueChanged, this, &LightingWidget::on_intensity_changed);
        connect(ui.slider_daylight_cct, &QSlider::valueChanged, this, &LightingWidget::on_daylight_changed);
        connect(ui.radioButton_d65, &QRadioButton::toggled, this, &LightingWidget::on_d65_toggled);
        connect(ui.radioButton_daylight, &QRadioButton::toggled, this, &LightingWidget::on_daylight_toggled);
}

void LightingWidget::set_view(view::View* view)
{
        m_view = view;
}

void LightingWidget::send_color()
{
        if (m_view)
        {
                m_view->send(view::command::SetLightingColor(rgb()));
        }
}

void LightingWidget::on_intensity_changed()
{
        m_intensity = position_to_intensity(slider_position(ui.slider_intensity));

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << m_intensity;
        ui.label_intensity->setText(QString::fromStdString(oss.str()));

        send_color();
}

void LightingWidget::on_d65_toggled()
{
        if (!ui.radioButton_d65->isChecked())
        {
                return;
        }

        m_spectrum = color::daylight_d65();
        m_rgb = color::Color(1, 1, 1);

        send_color();
}

void LightingWidget::on_daylight_toggled()
{
        const bool checked = ui.radioButton_daylight->isChecked();
        ui.slider_daylight_cct->setEnabled(checked);
        if (!checked)
        {
                return;
        }

        on_daylight_changed();
}

void LightingWidget::on_daylight_changed()
{
        const int cct = m_daylight_min_cct + DAYLIGHT_CCT_INTERVAL * ui.slider_daylight_cct->value();
        ASSERT(cct >= m_daylight_min_cct && cct <= m_daylight_max_cct);

        ui.radioButton_daylight->setText(QString::fromUtf8(DAYLIGHT_CCT.data(), DAYLIGHT_CCT.size()).arg(cct));

        if (!ui.radioButton_daylight->isChecked())
        {
                return;
        }

        m_spectrum = color::daylight(cct);
        m_rgb = m_spectrum.to_color<color::Color>();

        send_color();
}

color::Spectrum LightingWidget::spectrum() const
{
        return m_intensity * m_spectrum;
}

color::Color LightingWidget::rgb() const
{
        return m_intensity * m_rgb;
}

std::tuple<color::Spectrum, color::Color> LightingWidget::color() const
{
        return {m_intensity * m_spectrum, m_intensity * m_rgb};
}
}
