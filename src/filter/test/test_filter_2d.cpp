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

#include "position.h"
#include "position_estimation.h"
#include "position_filter_lkf.h"
#include "process.h"
#include "process_filter_ekf.h"
#include "process_filter_ukf.h"
#include "simulator.h"

#include "view/write.h"

#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/sort.h>
#include <src/test/test.h>

#include <cmath>
#include <optional>
#include <sstream>
#include <vector>

namespace ns::filter::test
{
namespace
{
template <typename T>
struct Config final
{
        static constexpr T POSITION_FILTER_VARIANCE = square(0.5);
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(10.0));
        static constexpr T POSITION_FILTER_ANGLE_ESTIMATION_TIME_DIFFERENCE = 1;
        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(0.1);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr std::array PROCESS_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});

        static constexpr T DATA_CONNECT_INTERVAL = 2;
};

template <typename T>
void write_to_file(
        const std::string_view annotation,
        const std::vector<Measurement<2, T>>& measurements,
        const std::vector<Position<T>>& positions,
        const std::vector<Process<T>>& processes)
{
        std::vector<view::Filter<2, T>> filters;
        filters.reserve(positions.size() + processes.size());

        for (const Position<T>& position : positions)
        {
                filters.push_back(
                        {.name = position.name(),
                         .color = position.color(),
                         .speed = position.speeds(),
                         .position = position.positions()});
        }

        for (const Process<T>& process : processes)
        {
                filters.push_back(
                        {.name = process.name(),
                         .color = process.color(),
                         .speed = process.speeds(),
                         .position = process.positions()});
        }

        view::write_to_file(annotation, measurements, Config<T>::DATA_CONNECT_INTERVAL, filters);
}

template <std::size_t N, typename T>
int compute_precision(const std::array<T, N>& data)
{
        std::optional<T> min;
        for (const T v : data)
        {
                ASSERT(v >= 0);
                if (!(v > 0))
                {
                        continue;
                }
                if (min)
                {
                        min = std::min(v, *min);
                        continue;
                }
                min = v;
        }
        if (!min)
        {
                return 0;
        }
        ASSERT(*min >= 1e-6L);
        return std::abs(std::floor(std::log10(*min)));
}

template <typename T>
class Positions final
{
        static std::vector<Position<T>> create_positions(
                const Vector<2, T>& init_position,
                const T init_position_variance)
        {
                const PositionFilterInit<T> init{
                        .position = init_position,
                        .position_variance = init_position_variance};

                std::vector<Position<T>> res;

                const int precision = compute_precision(Config<T>::POSITION_FILTER_THETAS);

                const auto name = [&](const T theta)
                {
                        const auto* const letter_theta = reinterpret_cast<const char*>(u8"\u03b8");
                        std::ostringstream oss;
                        oss << std::setprecision(precision) << std::fixed;
                        oss << "LKF (" << letter_theta << " " << theta << ")";
                        return oss.str();
                };

                const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
                for (std::size_t i = 0; i < thetas.size(); ++i)
                {
                        ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                        ASSERT(i <= 4);
                        res.emplace_back(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0),
                                create_position_filter_lkf(init, thetas[i], Config<T>::POSITION_FILTER_VARIANCE));
                }

                return res;
        }

        std::vector<Position<T>> positions_;
        std::optional<PositionEstimation<T>> estimation_;

public:
        Positions() = default;

        Positions(const Positions&) = delete;
        Positions& operator=(const Positions&) = delete;

        void update(const Measurement<2, T>& m)
        {
                if (!m.position)
                {
                        return;
                }
                if (positions_.empty())
                {
                        positions_ = create_positions(*m.position, m.position_variance);

                        estimation_.emplace(
                                Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_TIME_DIFFERENCE,
                                Config<T>::POSITION_FILTER_ANGLE_ESTIMATION_VARIANCE, &positions_);
                }

                for (auto& p : positions_)
                {
                        p.update(m);
                }

                estimation_->update(m);
        }

        [[nodiscard]] const std::vector<Position<T>>& positions() const
        {
                return positions_;
        }

        [[nodiscard]] bool has_estimates() const
        {
                if (estimation_ && estimation_->has_estimates())
                {
                        LOG(estimation_->description());
                        return true;
                }
                return false;
        }

        [[nodiscard]] T angle() const
        {
                ASSERT(estimation_);
                return estimation_->angle();
        }

        [[nodiscard]] Vector<2, T> position() const
        {
                ASSERT(estimation_);
                return estimation_->position();
        }

        [[nodiscard]] Vector<2, T> velocity() const
        {
                ASSERT(estimation_);
                return estimation_->velocity();
        }
};

template <typename T>
std::vector<Process<T>> create_processes(
        const Vector<2, T>& init_position,
        const Vector<2, T>& init_velocity,
        const T init_angle,
        const T init_position_variance)
{
        const ProcessFilterInit<T> init{
                .position = init_position,
                .velocity = init_velocity,
                .angle = init_angle,
                .position_variance = init_position_variance};

        const T process_pv = Config<T>::PROCESS_FILTER_POSITION_VARIANCE;
        const T process_av = Config<T>::PROCESS_FILTER_ANGLE_VARIANCE;
        const T process_arv = Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE;

        std::vector<Process<T>> res;

        res.emplace_back(
                "EKF", color::RGB8(0, 200, 0), create_process_filter_ekf<T>(init, process_pv, process_av, process_arv));

        const int precision = compute_precision(Config<T>::PROCESS_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                const auto* const letter_alpha = reinterpret_cast<const char*>(u8"\u03b1");
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "UKF (" << letter_alpha << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::PROCESS_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);
                res.emplace_back(
                        name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0),
                        create_process_filter_ukf(init, alphas[i], process_pv, process_av, process_arv));
        }

        return res;
}

template <typename T>
void write_result(
        const std::string_view annotation,
        const std::vector<Measurement<2, T>>& measurements,
        const std::vector<Position<T>>& positions,
        const std::vector<Process<T>>& processes)
{
        if (positions.empty())
        {
                error("No position measurement found");
        }

        if (processes.empty())
        {
                LOG("Failed to estimate direction");
        }

        write_to_file(annotation, measurements, positions, processes);

        for (const auto& p : positions)
        {
                LOG(p.nees_string());
        }

        for (const auto& p : processes)
        {
                LOG(p.nees_string());
        }
}

template <typename T>
void test_impl(const Track<2, T>& track)
{
        std::vector<Measurement<2, T>> measurements;
        Positions<T> positions;
        std::vector<Process<T>> processes;

        for (const Measurement<2, T>& m : track)
        {
                measurements.push_back(m);

                positions.update(m);

                if (processes.empty() && positions.has_estimates())
                {
                        processes = create_processes(
                                positions.position(), positions.velocity(), positions.angle(), m.position_variance);
                }

                for (auto& p : processes)
                {
                        p.update(m);
                }

                if (m.position)
                {
                        for (auto& p : processes)
                        {
                                LOG(to_string(m.time) + "; true angle = "
                                    + to_string(radians_to_degrees(m.true_data.angle)) + "; " + p.angle_string());
                        }
                }
        }

        write_result(track.annotation(), measurements, positions.positions(), processes);
}

template <typename T>
void test_impl()
{
        test_impl(track<2, T>());
}

void test()
{
        LOG("Test Filter 2D");
        LOG("---");
        test_impl<float>();
        LOG("---");
        test_impl<double>();
        LOG("---");
        test_impl<long double>();
        LOG("---");
        LOG("Test Filter 2D passed");
}

TEST_LARGE("Filter 2D", test)
}
}
