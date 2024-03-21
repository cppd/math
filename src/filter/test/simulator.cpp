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

#include "simulator.h"

#include <src/com/angle.h>
#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::filter::test
{
namespace
{
constexpr std::string_view DEGREE = "&#x00b0;";
constexpr std::string_view SIGMA = "&#x03c3;";

template <typename T>
[[nodiscard]] numerical::Vector<2, T> rotate(const numerical::Vector<2, T>& v, const T angle)
{
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const numerical::Matrix<2, 2, T> m{
                {cos, -sin},
                {sin,  cos}
        };
        return m * v;
}

template <typename T>
struct Config final
{
        std::size_t count = 8000;

        T speed_min = kph_to_mps(-30.0);
        T speed_max = kph_to_mps(130.0);
        T speed_clamp_min = kph_to_mps(0.0);
        T speed_clamp_max = kph_to_mps(100.0);
        T speed_variance = square(0.1);
        T velocity_magnitude_period = 110;
        T velocity_angle_period = 70;

        T angle = degrees_to_radians(-170.0);
        T angle_drift_per_hour = degrees_to_radians(-360.0);
        T angle_r = degrees_to_radians(30.0);

        T measurement_dt = 0.1L;
        unsigned measurement_dt_count_acceleration = 1;
        unsigned measurement_dt_count_direction = 1;
        unsigned measurement_dt_count_position = 10;
        unsigned measurement_dt_count_speed = 10;

        T measurement_variance_acceleration = square(1.0);
        T measurement_variance_direction = square(degrees_to_radians(2.0));
        T measurement_variance_position = square(25.0);
        T measurement_variance_speed = square(0.2);

        T bad_measurement_position = 1000;
        T bad_measurement_position_probability = T{0} / 20;
};

struct MeasurementInfo final
{
        bool position = false;
        bool speed = false;
        bool direction = false;
        bool acceleration = false;
};

template <std::size_t N, typename T>
MeasurementInfo measurement_info(const std::vector<filters::Measurements<N, T>>& measurements)
{
        MeasurementInfo res;

        for (const filters::Measurements<N, T>& m : measurements)
        {
                res.position = res.position || m.position.has_value();
                res.speed = res.speed || m.speed.has_value();
                res.direction = res.direction || m.direction.has_value();
                res.acceleration = res.acceleration || m.acceleration.has_value();
        }

        return res;
}

template <typename T>
void write_update_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / (config.measurement_dt * config.measurement_dt_count_position) << " Hz";

        if (measurement_info.speed)
        {
                oss << "<br>";
                oss << "speed: " << 1 / (config.measurement_dt * config.measurement_dt_count_speed) << " Hz";
        }

        if (measurement_info.direction)
        {
                oss << "<br>";
                oss << "direction: " << 1 / (config.measurement_dt * config.measurement_dt_count_direction) << " Hz";
        }

        if (measurement_info.acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << 1 / (config.measurement_dt * config.measurement_dt_count_acceleration)
                    << " Hz";
        }
}

template <typename T>
void write_bias_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        if (!measurement_info.direction && !measurement_info.acceleration)
        {
                return;
        }

        oss << "<br>";
        oss << "<br>";
        oss << "<b>bias</b>";
        oss << "<br>";
        oss << "direction drift: " << radians_to_degrees(config.angle_drift_per_hour) << " " << DEGREE << "/h";
        oss << "<br>";
        oss << "direction angle: " << radians_to_degrees(config.angle_r) << DEGREE;
}

template <typename T>
void write_sigma_annotation(const Config<T>& config, const MeasurementInfo& measurement_info, std::ostringstream& oss)
{
        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "position: " << std::sqrt(config.measurement_variance_position) << " m";

        if (measurement_info.speed)
        {
                oss << "<br>";
                oss << "speed: " << std::sqrt(config.measurement_variance_speed) << " m/s";
        }

        if (measurement_info.direction)
        {
                oss << "<br>";
                oss << "direction: " << radians_to_degrees(std::sqrt(config.measurement_variance_direction)) << DEGREE;
        }

        if (measurement_info.acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << std::sqrt(config.measurement_variance_acceleration) << " m/s<sup>2</sup>";
        }
}

template <std::size_t N, typename T>
std::string make_annotation(const Config<T>& config, const std::vector<filters::Measurements<N, T>>& measurements)
{
        const MeasurementInfo info = measurement_info(measurements);

        if (!info.position)
        {
                error("No position measurements");
        }

        std::ostringstream oss;

        write_update_annotation(config, info, oss);
        write_bias_annotation(config, info, oss);
        write_sigma_annotation(config, info, oss);

        return oss.str();
}

template <std::size_t N, typename T>
class Simulator final
{
        static constexpr T OFFSET = 500;

        struct Velocity final
        {
                T magnitude;
                T angle;
        };

        const T dt_;
        const T speed_m_;
        const T speed_a_;
        const T speed_clamp_min_;
        const T speed_clamp_max_;
        const T velocity_magnitude_period_;
        const T velocity_angle_period_;

        const T angle_drift_;
        const T angle_r_;

        const T bad_measurement_position_;
        const T bad_measurement_position_probability_;

        PCG engine_;

        std::normal_distribution<T> speed_nd_;

        std::normal_distribution<T> measurements_direction_nd_;
        std::normal_distribution<T> measurements_acceleration_nd_;
        std::normal_distribution<T> measurements_position_nd_;
        std::normal_distribution<T> measurements_speed_nd_;

        T time_{0};
        numerical::Vector<N, T> position_{0};
        Velocity velocity_;
        Velocity next_velocity_;
        numerical::Vector<N, T> acceleration_;
        T angle_;

        [[nodiscard]] T angle(const T time) const
        {
                if ((false))
                {
                        return std::cos(time * (2 * PI<T> / velocity_angle_period_));
                }

                static constexpr T PERIOD = 31;
                static constexpr T CHANGE_PERIOD = 9;
                const T period_number = std::floor(time / PERIOD);
                const T time_in_period = time - period_number * PERIOD;
                const T angle_time = period_number + std::clamp<T>(time_in_period / CHANGE_PERIOD, 0, 1);
                const T angle = -0.2 + (PI<T> / 2) * std::cos(angle_time * (PI<T> / 2));
                return angle;
        }

        [[nodiscard]] Velocity velocity_with_noise(const T time)
        {
                const T speed = speed_m_ + speed_a_ * std::sin(time * (2 * PI<T> / velocity_magnitude_period_));
                const T magnitude = std::clamp(speed, speed_clamp_min_, speed_clamp_max_);
                return {.magnitude = magnitude + speed_nd_(engine_), .angle = angle(time)};
        }

        [[nodiscard]] numerical::Vector<2, T> to_vector(const Velocity& v) const
        {
                return {v.magnitude * std::cos(v.angle), v.magnitude * std::sin(v.angle)};
        }

        [[nodiscard]] numerical::Vector<N, T> vector(std::normal_distribution<T>& distribution)
        {
                numerical::Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = distribution(engine_);
                }
                return res;
        }

public:
        explicit Simulator(const Config<T>& config)
                : dt_(config.measurement_dt),
                  speed_m_((config.speed_min + config.speed_max) / 2),
                  speed_a_((config.speed_max - config.speed_min) / 2),
                  speed_clamp_min_(config.speed_clamp_min),
                  speed_clamp_max_(config.speed_clamp_max),
                  velocity_magnitude_period_(config.velocity_magnitude_period),
                  velocity_angle_period_(config.velocity_angle_period),
                  angle_drift_(config.measurement_dt * config.angle_drift_per_hour / (T{60} * T{60})),
                  angle_r_(normalize_angle(config.angle_r)),
                  bad_measurement_position_(config.bad_measurement_position),
                  bad_measurement_position_probability_(config.bad_measurement_position_probability),
                  speed_nd_(0, std::sqrt(config.speed_variance)),
                  measurements_direction_nd_(0, std::sqrt(config.measurement_variance_direction)),
                  measurements_acceleration_nd_(0, std::sqrt(config.measurement_variance_acceleration)),
                  measurements_position_nd_(0, std::sqrt(config.measurement_variance_position)),
                  measurements_speed_nd_(0, std::sqrt(config.measurement_variance_speed)),
                  velocity_(velocity_with_noise(time_)),
                  next_velocity_(velocity_with_noise(time_ + dt_)),
                  acceleration_((to_vector(next_velocity_) - to_vector(velocity_)) / dt_),
                  angle_(normalize_angle(config.angle))
        {
                position_[N - 1] = OFFSET;
        }

        void move()
        {
                time_ += dt_;

                position_ = position_ + dt_ * to_vector(velocity_) + (square(dt_) / 2) * acceleration_;

                velocity_ = next_velocity_;
                next_velocity_ = velocity_with_noise(time_ + dt_);
                acceleration_ = (to_vector(next_velocity_) - to_vector(velocity_)) / dt_;

                angle_ = normalize_angle(angle_ + angle_drift_);
        }

        [[nodiscard]] const numerical::Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] T speed() const
        {
                return velocity_.magnitude;
        }

        [[nodiscard]] T angle() const
        {
                return angle_;
        }

        [[nodiscard]] T angle_r() const
        {
                return angle_r_;
        }

        [[nodiscard]] T measurement_direction()
        {
                return normalize_angle(velocity_.angle + angle_r_ + angle_ + measurements_direction_nd_(engine_));
        }

        [[nodiscard]] numerical::Vector<N, T> measurement_acceleration()
        {
                return rotate(acceleration_, angle_) + vector(measurements_acceleration_nd_);
        }

        [[nodiscard]] numerical::Vector<N, T> measurement_position()
        {
                const numerical::Vector<N, T> m = position_ + vector(measurements_position_nd_);
                if (std::bernoulli_distribution(bad_measurement_position_probability_)(engine_))
                {
                        return m + bad_measurement_position_ * sampling::uniform_on_sphere<N, T>(engine_);
                }
                return m;
        }

        [[nodiscard]] T measurement_speed()
        {
                return velocity_.magnitude + measurements_speed_nd_(engine_);
        }
};

template <std::size_t N, typename T>
std::vector<filters::Measurements<N, T>> simulate(const Config<T>& config)
{
        Simulator<N, T> simulator(config);

        std::vector<filters::Measurements<N, T>> measurements;
        measurements.reserve(config.count);

        for (std::size_t i = 0; i < config.count; ++i)
        {
                simulator.move();

                filters::Measurements<N, T>& m = measurements.emplace_back();

                m.true_data = {
                        .position = simulator.position(),
                        .speed = simulator.speed(),
                        .angle = simulator.angle(),
                        .angle_r = simulator.angle_r()};

                m.time = i * config.measurement_dt;

                if (i % config.measurement_dt_count_acceleration == 0)
                {
                        m.acceleration = {
                                .value = simulator.measurement_acceleration(),
                                .variance = numerical::Vector<N, T>(config.measurement_variance_acceleration)};
                }

                if (i % config.measurement_dt_count_direction == 0)
                {
                        m.direction = {
                                .value = numerical::Vector<1, T>(simulator.measurement_direction()),
                                .variance = numerical::Vector<1, T>(config.measurement_variance_direction)};
                }

                if (i % config.measurement_dt_count_position == 0)
                {
                        m.position = {
                                .value = simulator.measurement_position(),
                                .variance = numerical::Vector<N, T>(config.measurement_variance_position)};
                }

                if (i % config.measurement_dt_count_speed == 0)
                {
                        m.speed = {
                                .value = numerical::Vector<1, T>(simulator.measurement_speed()),
                                .variance = numerical::Vector<1, T>(config.measurement_variance_speed)};
                }
        }

        return measurements;
}

template <std::size_t N, typename T>
void correct_measurements(std::vector<filters::Measurements<N, T>>* const measurements)
{
        for (std::size_t i = 0; i < measurements->size(); ++i)
        {
                filters::Measurements<N, T>& m = (*measurements)[i];

                if (m.position)
                {
                        m.position->variance.reset();
                }

                const auto n = std::llround(m.time / 33);
                if ((n > 3) && ((n % 9) == 0))
                {
                        m.position.reset();
                        m.speed.reset();
                }

                if (std::llround(m.time / 10) % 8 == 0)
                {
                        m.speed.reset();
                }
        }
}
}

template <std::size_t N, typename T>
Track<N, T> track()
{
        const Config<T> config;

        ASSERT(config.speed_max >= config.speed_min);
        ASSERT(config.measurement_dt_count_acceleration > 0 && config.measurement_dt_count_direction > 0
               && config.measurement_dt_count_position > 0 && config.measurement_dt_count_speed > 0);

        std::vector<filters::Measurements<N, T>> measurements = simulate<N, T>(config);

        correct_measurements(&measurements);

        std::string annotation = make_annotation(config, measurements);

        return {std::move(measurements), std::move(annotation)};
}

#define TEMPLATE(T) template Track<2, T> track();

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
