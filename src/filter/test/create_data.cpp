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

#include "create_data.h"

#include "string.h"

#include "move/move_1_0.h"
#include "move/move_1_1.h"
#include "move/move_2_1.h"
#include "position/position_0.h"
#include "position/position_1.h"
#include "position/position_2.h"
#include "position/position_estimation.h"
#include "process/process_ekf.h"
#include "process/process_ukf.h"

#include <src/com/conversion.h>
#include <src/com/exponent.h>
#include <src/com/sort.h>

#include <array>
#include <sstream>

namespace ns::filter::test
{
namespace
{
template <typename T>
struct Config final
{
        static constexpr T POSITION_FILTER_VARIANCE_0 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_0{};

        static constexpr T POSITION_FILTER_VARIANCE_1 = square(1);
        static constexpr std::optional<T> POSITION_FILTER_GATE_1{10};

        static constexpr T POSITION_FILTER_VARIANCE_2 = square(0.5);
        static constexpr std::optional<T> POSITION_FILTER_GATE_2{5};

        static constexpr T POSITION_FILTER_MEASUREMENT_ANGLE_TIME_DIFFERENCE = 1;
        static constexpr std::array POSITION_FILTER_THETAS = std::to_array<T>({0});
        static constexpr T POSITION_FILTER_RESET_DT = 10;
        static constexpr T POSITION_FILTER_LINEAR_DT = 2;

        static constexpr T PROCESS_FILTER_POSITION_VARIANCE = square(1.0);
        static constexpr T PROCESS_FILTER_ANGLE_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_R_VARIANCE = square(degrees_to_radians(0.001));
        static constexpr T PROCESS_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array PROCESS_FILTER_UKF_ALPHAS = std::to_array<T>({0.1, 1.0});
        static constexpr T PROCESS_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> PROCESS_FILTER_GATE{};

        static constexpr T MOVE_FILTER_POSITION_VARIANCE_1_0 = square(2.0);
        static constexpr T MOVE_FILTER_POSITION_VARIANCE_1_1 = square(2.0);
        static constexpr T MOVE_FILTER_POSITION_VARIANCE_2_1 = square(1.0);
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_1_0 = square(degrees_to_radians(0.2));
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_1_1 = square(degrees_to_radians(0.001));
        static constexpr T MOVE_FILTER_ANGLE_VARIANCE_2_1 = square(degrees_to_radians(0.001));
        static constexpr T MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE = square(degrees_to_radians(20.0));
        static constexpr std::array MOVE_FILTER_UKF_ALPHAS = std::to_array<T>({1.0});
        static constexpr T MOVE_FILTER_RESET_DT = 10;
        static constexpr std::optional<T> MOVE_FILTER_GATE{};
};

template <std::size_t N, typename T>
std::vector<position::PositionVariance<N, T>> create_position_variance()
{
        std::vector<position::PositionVariance<N, T>> res;

        res.emplace_back(
                "Variance LKF", color::RGB8(0, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                Config<T>::POSITION_FILTER_VARIANCE_2);

        return res;
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<std::unique_ptr<position::Position<N, T>>> create_positions()
{
        std::vector<std::unique_ptr<position::Position<N, T>>> res;

        const int precision = compute_string_precision(Config<T>::POSITION_FILTER_THETAS);

        const auto name = [&](const T theta)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "LKF " << ORDER << " (" << THETA << " " << theta << ")";
                return oss.str();
        };

        const auto thetas = sort(std::array(Config<T>::POSITION_FILTER_THETAS));
        for (std::size_t i = 0; i < thetas.size(); ++i)
        {
                ASSERT(thetas[i] >= 0 && thetas[i] <= 1);
                ASSERT(i <= 4);

                static_assert(ORDER >= 0 && ORDER <= 2);

                if (ORDER == 0)
                {
                        res.emplace_back(std::make_unique<position::Position0<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 100, 200),
                                Config<T>::POSITION_FILTER_RESET_DT, Config<T>::POSITION_FILTER_LINEAR_DT,
                                Config<T>::POSITION_FILTER_GATE_0, thetas[i], Config<T>::POSITION_FILTER_VARIANCE_0));
                }

                if (ORDER == 1)
                {
                        res.emplace_back(std::make_unique<position::Position1<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 200), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_1, thetas[i],
                                Config<T>::POSITION_FILTER_VARIANCE_1));
                }

                if (ORDER == 2)
                {
                        res.emplace_back(std::make_unique<position::Position2<N, T>>(
                                name(thetas[i]), color::RGB8(160 - 40 * i, 0, 0), Config<T>::POSITION_FILTER_RESET_DT,
                                Config<T>::POSITION_FILTER_LINEAR_DT, Config<T>::POSITION_FILTER_GATE_2, thetas[i],
                                Config<T>::POSITION_FILTER_VARIANCE_2));
                }
        }

        return res;
}

template <typename T>
std::vector<std::unique_ptr<process::Process<T>>> create_processes()
{
        const T process_pv = Config<T>::PROCESS_FILTER_POSITION_VARIANCE;
        const T process_av = Config<T>::PROCESS_FILTER_ANGLE_VARIANCE;
        const T process_arv = Config<T>::PROCESS_FILTER_ANGLE_R_VARIANCE;

        std::vector<std::unique_ptr<process::Process<T>>> res;

        res.push_back(std::make_unique<process::ProcessEkf<T>>(
                "EKF", color::RGB8(0, 200, 0), Config<T>::PROCESS_FILTER_RESET_DT,
                Config<T>::PROCESS_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::PROCESS_FILTER_GATE, process_pv,
                process_av, process_arv));

        const int precision = compute_string_precision(Config<T>::PROCESS_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "UKF (" << ALPHA << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::PROCESS_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                ASSERT(alphas[i] > 0 && alphas[i] <= 1);
                ASSERT(i <= 4);
                res.push_back(std::make_unique<process::ProcessUkf<T>>(
                        name(alphas[i]), color::RGB8(0, 160 - 40 * i, 0), Config<T>::PROCESS_FILTER_RESET_DT,
                        Config<T>::PROCESS_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::PROCESS_FILTER_GATE, alphas[i],
                        process_pv, process_av, process_arv));
        }

        return res;
}

template <typename T, std::size_t ORDER_P, std::size_t ORDER_A>
std::unique_ptr<move::Move<T>> create_move(const unsigned i, const T alpha, const std::string& name)
{
        ASSERT(alpha > 0 && alpha <= 1);
        ASSERT(i <= 4);

        static_assert((ORDER_P == 1 && (ORDER_A == 0 || ORDER_A == 1)) || (ORDER_P == 2 && ORDER_A == 1));

        if (ORDER_P == 1 && ORDER_A == 0)
        {
                return std::make_unique<move::Move10<T>>(
                        name, color::RGB8(0, 160 - 40 * i, 250), Config<T>::MOVE_FILTER_RESET_DT,
                        Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE, alpha,
                        Config<T>::MOVE_FILTER_POSITION_VARIANCE_1_0, Config<T>::MOVE_FILTER_ANGLE_VARIANCE_1_0);
        }

        if (ORDER_P == 1 && ORDER_A == 1)
        {
                return std::make_unique<move::Move11<T>>(
                        name, color::RGB8(0, 160 - 40 * i, 150), Config<T>::MOVE_FILTER_RESET_DT,
                        Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE, alpha,
                        Config<T>::MOVE_FILTER_POSITION_VARIANCE_1_1, Config<T>::MOVE_FILTER_ANGLE_VARIANCE_1_1);
        }

        if (ORDER_P == 2 && ORDER_A == 1)
        {
                return std::make_unique<move::Move21<T>>(
                        name, color::RGB8(0, 160 - 40 * i, 50), Config<T>::MOVE_FILTER_RESET_DT,
                        Config<T>::MOVE_FILTER_ANGLE_ESTIMATION_VARIANCE, Config<T>::MOVE_FILTER_GATE, alpha,
                        Config<T>::MOVE_FILTER_POSITION_VARIANCE_2_1, Config<T>::MOVE_FILTER_ANGLE_VARIANCE_2_1);
        }
}

template <typename T, std::size_t ORDER_P, std::size_t ORDER_A>
std::vector<std::unique_ptr<move::Move<T>>> create_moves()
{
        std::vector<std::unique_ptr<move::Move<T>>> res;

        const int precision = compute_string_precision(Config<T>::MOVE_FILTER_UKF_ALPHAS);

        const auto name = [&](const T alpha)
        {
                std::ostringstream oss;
                oss << std::setprecision(precision) << std::fixed;
                oss << "Move UKF " << ORDER_P << '.' << ORDER_A << " (" << ALPHA << " " << alpha << ")";
                return oss.str();
        };

        const auto alphas = sort(std::array(Config<T>::MOVE_FILTER_UKF_ALPHAS));
        for (std::size_t i = 0; i < alphas.size(); ++i)
        {
                res.push_back(create_move<T, ORDER_P, ORDER_A>(i, alphas[i], name(alphas[i])));
        }

        return res;
}
}

template <typename T>
Test<T> create_data()
{
        Test<T> res;

        res.position_variance = create_position_variance<2, T>();

        res.positions_0 = create_positions<2, T, 0>();
        res.positions_1 = create_positions<2, T, 1>();
        res.positions_2 = create_positions<2, T, 2>();

        res.processes = create_processes<T>();

        res.moves_1_0 = create_moves<T, 1, 0>();
        res.moves_1_1 = create_moves<T, 1, 1>();
        res.moves_2_1 = create_moves<T, 2, 1>();

        res.position_estimation = std::make_unique<position::PositionEstimation<T>>(
                Config<T>::POSITION_FILTER_MEASUREMENT_ANGLE_TIME_DIFFERENCE,
                static_cast<const position::Position2<2, T>*>(res.positions_2.front().get()));

        return res;
}

#define TEMPLATE_T(T) template Test<T> create_data();

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
