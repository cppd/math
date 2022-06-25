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

constexpr std::string_view DAYLIGHT_D65 = "Daylight D65";
constexpr std::string_view DAYLIGHT_CCT = "Daylight %1K";

constexpr double BLACKBODY_MIN_T = 1000;
constexpr double BLACKBODY_MAX_T = 25000;
constexpr std::string_view BLACKBODY_A = "Blackbody A";
constexpr std::string_view BLACKBODY_T = "Blackbody %1K";

constexpr double TEMPERATURE_ROUND = 10;

constexpr double FRONT_LIGHTING_PROPORTION = 0.2;

double ceil(const double v, const int t)
{
        return std::ceil(v / t) * t;
}

double floor(const double v, const int t)
{
        return std::floor(v / t) * t;
}

double position_to_intensity(const double position)
{
        const double v = 2 * position - 1;
        const double intensity = std::pow(MAXIMUM_LIGHTING_INTENSITY, v);
        return std::clamp(intensity, 1 / MAXIMUM_LIGHTING_INTENSITY, MAXIMUM_LIGHTING_INTENSITY);
}

double intensity_to_position(const double intensity)
{
        const double position = std::log(intensity) / std::log(MAXIMUM_LIGHTING_INTENSITY);
        return std::clamp((position + 1) / 2, 0.0, 1.0);
}

int position_to_temperature(const double position, const double min, const double max)
{
        ASSERT(min > 0 && min < max);
        double t = min * std::pow(max / min, position);
        t = std::round(t / TEMPERATURE_ROUND) * TEMPERATURE_ROUND;
        return std::clamp(t, min, max);
}
}

LightingWidget::LightingWidget()
        : QWidget(nullptr),
          daylight_min_cct_(ceil(color::daylight_min_cct(), TEMPERATURE_ROUND)),
          daylight_max_cct_(floor(color::daylight_max_cct(), TEMPERATURE_ROUND)),
          blackbody_min_t_(ceil(BLACKBODY_MIN_T, TEMPERATURE_ROUND)),
          blackbody_max_t_(floor(BLACKBODY_MAX_T, TEMPERATURE_ROUND))
{
        if (!(daylight_max_cct_ > daylight_min_cct_))
        {
                error("Error daylight CCT min " + to_string(daylight_min_cct_) + " and max "
                      + to_string(daylight_max_cct_));
        }

        if (!(blackbody_max_t_ > blackbody_min_t_))
        {
                error("Error blackbody T min " + to_string(blackbody_min_t_) + " and max "
                      + to_string(blackbody_max_t_));
        }

        ui_.setupUi(this);

        ui_.slider_intensity->setMinimum(0);
        ui_.slider_intensity->setMaximum(1000);
        set_slider_position(ui_.slider_intensity, intensity_to_position(DEFAULT_LIGHTING_INTENSITY));

        ui_.slider_daylight_cct->setMinimum(0);
        ui_.slider_daylight_cct->setMaximum(1000);
        set_slider_to_middle(ui_.slider_daylight_cct);

        ui_.slider_blackbody_t->setMinimum(0);
        ui_.slider_blackbody_t->setMaximum(1000);
        set_slider_to_middle(ui_.slider_blackbody_t);

        ui_.radioButton_d65->setChecked(true);
        ui_.radioButton_d65->setText(QString::fromUtf8(DAYLIGHT_D65.data(), DAYLIGHT_D65.size()));
        ui_.radioButton_blackbody_a->setText(QString::fromUtf8(BLACKBODY_A.data(), BLACKBODY_A.size()));

        ui_.slider_front_lighting->setMinimum(0);
        ui_.slider_front_lighting->setMaximum(100);
        set_slider_position(ui_.slider_front_lighting, FRONT_LIGHTING_PROPORTION);

        on_intensity_changed();
        on_daylight_changed();
        on_blackbody_changed();
        on_d65_toggled();
        on_daylight_toggled();
        on_blackbody_a_toggled();
        on_blackbody_toggled();
        on_front_lighting_changed();

        connect(ui_.slider_intensity, &QSlider::valueChanged, this, &LightingWidget::on_intensity_changed);
        connect(ui_.slider_daylight_cct, &QSlider::valueChanged, this, &LightingWidget::on_daylight_changed);
        connect(ui_.slider_blackbody_t, &QSlider::valueChanged, this, &LightingWidget::on_blackbody_changed);
        connect(ui_.radioButton_d65, &QRadioButton::toggled, this, &LightingWidget::on_d65_toggled);
        connect(ui_.radioButton_daylight, &QRadioButton::toggled, this, &LightingWidget::on_daylight_toggled);
        connect(ui_.radioButton_blackbody_a, &QRadioButton::toggled, this, &LightingWidget::on_blackbody_a_toggled);
        connect(ui_.radioButton_blackbody, &QRadioButton::toggled, this, &LightingWidget::on_blackbody_toggled);
        connect(ui_.slider_front_lighting, &QSlider::valueChanged, this, &LightingWidget::on_front_lighting_changed);
}

void LightingWidget::set_view(view::View* const view)
{
        view_ = view;
}

void LightingWidget::send_color()
{
        if (view_)
        {
                view_->send(view::command::SetLightingColor(rgb()));
        }
}

void LightingWidget::on_intensity_changed()
{
        intensity_ = position_to_intensity(slider_position(ui_.slider_intensity));

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << intensity_;
        ui_.label_intensity->setText(QString::fromStdString(oss.str()));

        send_color();
}

void LightingWidget::on_d65_toggled()
{
        if (!ui_.radioButton_d65->isChecked())
        {
                return;
        }

        spectrum_ = color::daylight_d65();
        rgb_ = color::Color(1, 1, 1);

        send_color();
}

void LightingWidget::on_daylight_toggled()
{
        const bool checked = ui_.radioButton_daylight->isChecked();
        ui_.slider_daylight_cct->setEnabled(checked);
        if (!checked)
        {
                return;
        }

        on_daylight_changed();
}

void LightingWidget::on_daylight_changed()
{
        const double p = slider_position(ui_.slider_daylight_cct);
        const int cct = position_to_temperature(p, daylight_min_cct_, daylight_max_cct_);

        ui_.radioButton_daylight->setText(QString::fromUtf8(DAYLIGHT_CCT.data(), DAYLIGHT_CCT.size()).arg(cct));

        if (!ui_.radioButton_daylight->isChecked())
        {
                return;
        }

        spectrum_ = color::daylight(cct);
        rgb_ = to_color<color::Color>(spectrum_);

        send_color();
}

void LightingWidget::on_blackbody_a_toggled()
{
        if (!ui_.radioButton_blackbody_a->isChecked())
        {
                return;
        }

        spectrum_ = color::blackbody_a();
        rgb_ = to_color<color::Color>(spectrum_);

        send_color();
}

void LightingWidget::on_blackbody_toggled()
{
        const bool checked = ui_.radioButton_blackbody->isChecked();
        ui_.slider_blackbody_t->setEnabled(checked);
        if (!checked)
        {
                return;
        }

        on_blackbody_changed();
}

void LightingWidget::on_blackbody_changed()
{
        const double p = slider_position(ui_.slider_blackbody_t);
        const int t = position_to_temperature(p, blackbody_min_t_, blackbody_max_t_);

        ui_.radioButton_blackbody->setText(QString::fromUtf8(BLACKBODY_T.data(), BLACKBODY_T.size()).arg(t));

        if (!ui_.radioButton_blackbody->isChecked())
        {
                return;
        }

        spectrum_ = color::blackbody(t);
        rgb_ = to_color<color::Color>(spectrum_);

        send_color();
}

void LightingWidget::on_front_lighting_changed()
{
        front_lighting_proportion_ = slider_position(ui_.slider_front_lighting);

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << front_lighting_proportion_;
        ui_.label_front_lighting->setText(QString::fromStdString(oss.str()));

        if (view_)
        {
                view_->send(view::command::SetFrontLightingProportion(front_lighting_proportion_));
        }
}

std::vector<view::Command> LightingWidget::commands() const
{
        return {view::command::SetLightingColor(rgb()),
                view::command::SetFrontLightingProportion(front_lighting_proportion())};
}

color::Spectrum LightingWidget::spectrum() const
{
        return intensity_ * spectrum_;
}

color::Color LightingWidget::rgb() const
{
        return intensity_ * rgb_;
}

std::tuple<color::Spectrum, color::Color> LightingWidget::color() const
{
        return {spectrum(), rgb()};
}

double LightingWidget::front_lighting_proportion() const
{
        return front_lighting_proportion_;
}
}
