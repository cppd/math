/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "utility.h"

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>

#include <cmath>
#include <random>

namespace ns::filter::test
{
namespace
{
template <typename T>
struct Config final
{
        std::size_t count = 8000;

        T speed_min = 0;
        T speed_max = kph_to_mps(100.0);
        T speed_variance = square(0.1);
        T velocity_magnitude_period = 60;
        T velocity_angle_period = 90;

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
        T measurement_variance_position = square(20.0);
        T measurement_variance_speed = square(0.2);
};

template <std::size_t N, typename T>
std::string make_annotation(const Config<T>& config, const std::vector<Measurement<N, T>>& measurements)
{
        bool position = false;
        bool speed = false;
        bool direction = false;
        bool acceleration = false;

        for (const Measurement<N, T>& m : measurements)
        {
                position = position || m.position.has_value();
                speed = speed || m.speed.has_value();
                direction = direction || m.direction.has_value();
                acceleration = acceleration || m.acceleration.has_value();
        }

        if (!position)
        {
                error("No position measurements");
        }

        constexpr std::string_view DEGREE = "&#x00b0;";
        constexpr std::string_view SIGMA = "&#x03c3;";
        std::ostringstream oss;

        oss << "<b>update</b>";
        oss << "<br>";
        oss << "position: " << 1 / (config.measurement_dt * config.measurement_dt_count_position) << " Hz";
        if (speed)
        {
                oss << "<br>";
                oss << "speed: " << 1 / (config.measurement_dt * config.measurement_dt_count_speed) << " Hz";
        }
        if (direction)
        {
                oss << "<br>";
                oss << "direction: " << 1 / (config.measurement_dt * config.measurement_dt_count_direction) << " Hz";
        }
        if (acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << 1 / (config.measurement_dt * config.measurement_dt_count_acceleration)
                    << " Hz";
        }
        if (direction || acceleration)
        {
                oss << "<br>";
                oss << "<br>";
                oss << "<b>bias</b>";
                oss << "<br>";
                oss << "direction drift: " << radians_to_degrees(config.angle_drift_per_hour) << " " << DEGREE << "/h";
                oss << "<br>";
                oss << "direction angle: " << radians_to_degrees(config.angle_r) << DEGREE;
        }
        oss << "<br>";
        oss << "<br>";
        oss << "<b>" << SIGMA << "</b>";
        oss << "<br>";
        oss << "position: " << std::sqrt(config.measurement_variance_position) << " m";
        if (speed)
        {
                oss << "<br>";
                oss << "speed: " << std::sqrt(config.measurement_variance_speed) << " m/s";
        }
        if (direction)
        {
                oss << "<br>";
                oss << "direction: " << radians_to_degrees(std::sqrt(config.measurement_variance_direction)) << DEGREE;
        }
        if (acceleration)
        {
                oss << "<br>";
                oss << "acceleration: " << std::sqrt(config.measurement_variance_acceleration) << " m/s<sup>2</sup>";
        }

        return oss.str();
}

template <std::size_t N, typename T>
class Simulator final
{
        struct Velocity final
        {
                T magnitude;
                T angle;
        };

        const T dt_;
        const T speed_m_;
        const T speed_a_;
        const T velocity_magnitude_period_;
        const T velocity_angle_period_;

        const T angle_drift_;
        const T angle_r_;

        PCG engine_;

        std::normal_distribution<T> speed_nd_;

        std::normal_distribution<T> measurements_direction_nd_;
        std::normal_distribution<T> measurements_acceleration_nd_;
        std::normal_distribution<T> measurements_position_nd_;
        std::normal_distribution<T> measurements_speed_nd_;

        T time_{0};
        Vector<N, T> position_{0};
        Velocity velocity_;
        Velocity next_velocity_;
        Vector<N, T> acceleration_;
        T angle_;

        [[nodiscard]] Velocity velocity_with_noise(const T time)
        {
                static constexpr T S = 100;
                const T magnitude =
                        std::max<T>(0, speed_m_ + speed_a_ * std::sin(time * (2 * PI<T> / velocity_magnitude_period_)));
                const T angle = std::cos((std::max<T>(0, time - S)) * (2 * PI<T> / velocity_angle_period_));
                return {.magnitude = magnitude + speed_nd_(engine_), .angle = angle};
        }

        [[nodiscard]] Vector<2, T> to_vector(const Velocity& v) const
        {
                return {v.magnitude * std::cos(v.angle), v.magnitude * std::sin(v.angle)};
        }

        [[nodiscard]] Vector<N, T> vector(std::normal_distribution<T>& distribution)
        {
                Vector<N, T> res;
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
                  velocity_magnitude_period_(config.velocity_magnitude_period),
                  velocity_angle_period_(config.velocity_angle_period),
                  angle_drift_(config.measurement_dt * config.angle_drift_per_hour / (T{60} * T{60})),
                  angle_r_(normalize_angle(config.angle_r)),
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

        [[nodiscard]] const Vector<N, T>& position() const
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

        [[nodiscard]] Vector<N, T> measurement_acceleration()
        {
                return rotate(acceleration_, angle_) + vector(measurements_acceleration_nd_);
        }

        [[nodiscard]] Vector<N, T> measurement_position()
        {
                return position_ + vector(measurements_position_nd_);
        }

        [[nodiscard]] T measurement_speed()
        {
                return velocity_.magnitude + measurements_speed_nd_(engine_);
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track()
{
        const Config<T> config;

        ASSERT(config.speed_max >= config.speed_min);
        ASSERT(config.measurement_dt_count_acceleration > 0 && config.measurement_dt_count_direction > 0
               && config.measurement_dt_count_position > 0 && config.measurement_dt_count_speed > 0);

        Simulator<N, T> simulator(config);

        Track<N, T> res;
        res.measurements.reserve(config.count);

        for (std::size_t i = 0; i < config.count; ++i)
        {
                simulator.move();

                Measurement<N, T>& m = res.measurements.emplace_back();

                m.true_data = {
                        .position = simulator.position(),
                        .speed = simulator.speed(),
                        .angle = simulator.angle(),
                        .angle_r = simulator.angle_r()};

                m.time = i * config.measurement_dt;

                if (i % config.measurement_dt_count_acceleration == 0)
                {
                        m.acceleration = simulator.measurement_acceleration();
                }
                m.acceleration_variance = config.measurement_variance_acceleration;

                if (i % config.measurement_dt_count_direction == 0)
                {
                        m.direction = simulator.measurement_direction();
                }
                m.direction_variance = config.measurement_variance_direction;

                if (i % config.measurement_dt_count_position == 0)
                {
                        m.position = simulator.measurement_position();
                }
                m.position_variance = config.measurement_variance_position;

                if (i % config.measurement_dt_count_speed == 0)
                {
                        m.speed = simulator.measurement_speed();
                }
                m.speed_variance = config.measurement_variance_speed;

                const auto n = std::llround(m.time / 33);
                if ((n > 3) && ((n % 5) == 0))
                {
                        m.position.reset();
                        m.speed.reset();
                }

                if ((std::llround(m.time / 10) % 8) == 0)
                {
                        m.speed.reset();
                }
        }

        res.annotation = make_annotation(config, res.measurements);

        return res;
}

#define TEMPLATE(T) template Track<2, T> generate_track();

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
