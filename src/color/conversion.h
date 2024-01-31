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

#pragma once

#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace ns::color
{
namespace conversion_implementation
{
template <typename UInt>
constexpr float uint_to_float(const UInt c)
{
        static_assert(std::is_same_v<UInt, std::uint8_t> || std::is_same_v<UInt, std::uint16_t>);

        constexpr float MAX = Limits<UInt>::max();
        return c / MAX;
}

template <typename UInt, typename T>
constexpr UInt float_to_uint(const T c)
{
        static_assert(std::is_same_v<UInt, std::uint8_t> || std::is_same_v<UInt, std::uint16_t>);
        static_assert(std::is_same_v<T, float>);

        constexpr float MAX = Limits<UInt>::max();
        return c * MAX + 0.5f;
}

template <typename UInt, typename T>
constexpr UInt float_clamp_to_uint(const T c)
{
        static_assert(std::is_same_v<UInt, std::uint8_t> || std::is_same_v<UInt, std::uint16_t>);
        static_assert(std::is_same_v<T, float>);

        if (c < 1)
        {
                if (c > 0)
                {
                        return float_to_uint<UInt>(c);
                }
                return 0;
        }
        return Limits<UInt>::max();
}

// clang-format off
inline constexpr std::array<float, 256> SRGB_UINT8_TO_RGB_FLOAT =
{
        0.000000000e+00, 3.035269910e-04, 6.070539821e-04, 9.105809731e-04,
        1.214107964e-03, 1.517634955e-03, 1.821161946e-03, 2.124688821e-03,
        2.428215928e-03, 2.731742803e-03, 3.035269910e-03, 3.346535843e-03,
        3.676507389e-03, 4.024717025e-03, 4.391442053e-03, 4.776953254e-03,
        5.181516521e-03, 5.605391692e-03, 6.048833020e-03, 6.512090564e-03,
        6.995410193e-03, 7.499032188e-03, 8.023193106e-03, 8.568125777e-03,
        9.134058841e-03, 9.721217677e-03, 1.032982301e-02, 1.096009370e-02,
        1.161224488e-02, 1.228648797e-02, 1.298303250e-02, 1.370208338e-02,
        1.444384363e-02, 1.520851441e-02, 1.599629410e-02, 1.680737548e-02,
        1.764195412e-02, 1.850022003e-02, 1.938236132e-02, 2.028856240e-02,
        2.121900953e-02, 2.217388526e-02, 2.315336652e-02, 2.415763214e-02,
        2.518685907e-02, 2.624122240e-02, 2.732089162e-02, 2.842603996e-02,
        2.955683507e-02, 3.071344458e-02, 3.189603239e-02, 3.310476616e-02,
        3.433980793e-02, 3.560131416e-02, 3.688944876e-02, 3.820437193e-02,
        3.954623640e-02, 4.091519862e-02, 4.231141135e-02, 4.373503104e-02,
        4.518620297e-02, 4.666508734e-02, 4.817182571e-02, 4.970656708e-02,
        5.126945674e-02, 5.286064744e-02, 5.448027700e-02, 5.612849072e-02,
        5.780543014e-02, 5.951123685e-02, 6.124605238e-02, 6.301001459e-02,
        6.480326504e-02, 6.662593782e-02, 6.847816706e-02, 7.036009431e-02,
        7.227185369e-02, 7.421357185e-02, 7.618538290e-02, 7.818742096e-02,
        8.021982014e-02, 8.228270710e-02, 8.437620848e-02, 8.650045842e-02,
        8.865558356e-02, 9.084171057e-02, 9.305896610e-02, 9.530746937e-02,
        9.758734703e-02, 9.989872575e-02, 1.022417322e-01, 1.046164855e-01,
        1.070231050e-01, 1.094617099e-01, 1.119324267e-01, 1.144353747e-01,
        1.169706658e-01, 1.195384264e-01, 1.221387759e-01, 1.247718185e-01,
        1.274376810e-01, 1.301364750e-01, 1.328683197e-01, 1.356333345e-01,
        1.384316087e-01, 1.412632912e-01, 1.441284716e-01, 1.470272690e-01,
        1.499597877e-01, 1.529261470e-01, 1.559264660e-01, 1.589608341e-01,
        1.620293707e-01, 1.651321948e-01, 1.682693958e-01, 1.714411080e-01,
        1.746474057e-01, 1.778884232e-01, 1.811642498e-01, 1.844749898e-01,
        1.878207773e-01, 1.912016869e-01, 1.946178377e-01, 1.980693191e-01,
        2.015562505e-01, 2.050787359e-01, 2.086368650e-01, 2.122307569e-01,
        2.158605009e-01, 2.195262015e-01, 2.232279629e-01, 2.269658744e-01,
        2.307400554e-01, 2.345505804e-01, 2.383975685e-01, 2.422811240e-01,
        2.462013215e-01, 2.501582801e-01, 2.541520894e-01, 2.581828535e-01,
        2.622506618e-01, 2.663556039e-01, 2.704977989e-01, 2.746773064e-01,
        2.788942754e-01, 2.831487358e-01, 2.874408364e-01, 2.917706370e-01,
        2.961382568e-01, 3.005437851e-01, 3.049873114e-01, 3.094689250e-01,
        3.139887154e-01, 3.185467720e-01, 3.231432140e-01, 3.277781010e-01,
        3.324515224e-01, 3.371636271e-01, 3.419144154e-01, 3.467040658e-01,
        3.515326083e-01, 3.564001322e-01, 3.613067865e-01, 3.662526011e-01,
        3.712376952e-01, 3.762621284e-01, 3.813260198e-01, 3.864294291e-01,
        3.915724754e-01, 3.967552185e-01, 4.019777775e-01, 4.072402120e-01,
        4.125426114e-01, 4.178850651e-01, 4.232676625e-01, 4.286904931e-01,
        4.341536462e-01, 4.396571815e-01, 4.452011883e-01, 4.507857859e-01,
        4.564110339e-01, 4.620769918e-01, 4.677838087e-01, 4.735314846e-01,
        4.793201685e-01, 4.851499498e-01, 4.910208583e-01, 4.969329834e-01,
        5.028864741e-01, 5.088813305e-01, 5.149176717e-01, 5.209955573e-01,
        5.271151066e-01, 5.332763791e-01, 5.394794941e-01, 5.457244515e-01,
        5.520114303e-01, 5.583403707e-01, 5.647115111e-01, 5.711248517e-01,
        5.775804520e-01, 5.840784311e-01, 5.906188488e-01, 5.972017646e-01,
        6.038273573e-01, 6.104955673e-01, 6.172065735e-01, 6.239603758e-01,
        6.307571530e-01, 6.375968456e-01, 6.444796920e-01, 6.514056325e-01,
        6.583748460e-01, 6.653872728e-01, 6.724431515e-01, 6.795424819e-01,
        6.866853237e-01, 6.938717365e-01, 7.011018991e-01, 7.083757520e-01,
        7.156934738e-01, 7.230551243e-01, 7.304607630e-01, 7.379103899e-01,
        7.454041839e-01, 7.529422045e-01, 7.605245113e-01, 7.681511641e-01,
        7.758222222e-01, 7.835378051e-01, 7.912979126e-01, 7.991027236e-01,
        8.069522381e-01, 8.148465753e-01, 8.227857351e-01, 8.307698965e-01,
        8.387989998e-01, 8.468732238e-01, 8.549926281e-01, 8.631572127e-01,
        8.713670969e-01, 8.796223998e-01, 8.879231215e-01, 8.962693810e-01,
        9.046611786e-01, 9.130986333e-01, 9.215818644e-01, 9.301108718e-01,
        9.386857152e-01, 9.473065138e-01, 9.559733272e-01, 9.646862745e-01,
        9.734452963e-01, 9.822505713e-01, 9.911020994e-01, 1.000000000e+00
};
// clang-format on
}

//

template <typename T>
T linear_float_to_srgb_float(const T c)
{
        static_assert(std::is_floating_point_v<T>);

        if (c >= 1)
        {
                return 1;
        }
        if (c >= T{0.0031308})
        {
                return T{1.055} * std::pow(c, T{1} / T{2.4}) - T{0.055};
        }
        if (c > 0)
        {
                return c * T{12.92};
        }
        return 0;
}

template <typename T>
T srgb_float_to_linear_float(const T c)
{
        static_assert(std::is_floating_point_v<T>);

        if (c >= 1)
        {
                return 1;
        }
        if (c >= T{0.04045})
        {
                return std::pow((c + T{0.055}) / T{1.055}, T{2.4});
        }
        if (c > 0)
        {
                return c / T{12.92};
        }
        return 0;
}

//

template <typename UInt8>
constexpr float srgb_uint8_to_linear_float(const UInt8 c)
{
        namespace impl = conversion_implementation;

        static_assert(std::is_same_v<UInt8, std::uint8_t>);
        static_assert(std::is_same_v<float, std::remove_cvref_t<decltype(impl::SRGB_UINT8_TO_RGB_FLOAT[c])>>);

        return impl::SRGB_UINT8_TO_RGB_FLOAT[c];
}

template <typename UInt8>
constexpr float linear_uint8_to_linear_float(const UInt8 c)
{
        static_assert(std::is_same_v<UInt8, std::uint8_t>);

        return conversion_implementation::uint_to_float(c);
}

template <typename T>
std::uint8_t linear_float_to_srgb_uint8(const T c)
{
        static_assert(std::is_same_v<T, float>);

        return conversion_implementation::float_to_uint<std::uint8_t>(linear_float_to_srgb_float(c));
}

template <typename T>
constexpr std::uint8_t linear_float_to_linear_uint8(const T c)
{
        static_assert(std::is_same_v<T, float>);

        return conversion_implementation::float_clamp_to_uint<std::uint8_t>(c);
}

//

template <typename T>
float srgb_uint16_to_linear_float(const T c)
{
        static_assert(std::is_same_v<T, std::uint16_t>);

        return srgb_float_to_linear_float(conversion_implementation::uint_to_float(c));
}

template <typename T>
constexpr float linear_uint16_to_linear_float(const T c)
{
        static_assert(std::is_same_v<T, std::uint16_t>);

        return conversion_implementation::uint_to_float(c);
}

template <typename T>
std::uint16_t linear_float_to_srgb_uint16(const T c)
{
        static_assert(std::is_same_v<T, float>);

        return conversion_implementation::float_to_uint<std::uint16_t>(linear_float_to_srgb_float(c));
}

template <typename T>
constexpr std::uint16_t linear_float_to_linear_uint16(const T c)
{
        static_assert(std::is_same_v<T, float>);

        return conversion_implementation::float_clamp_to_uint<std::uint16_t>(c);
}

//

template <typename T>
constexpr T linear_float_to_linear_luminance(const T red, const T green, const T blue)
{
        static_assert(std::is_floating_point_v<T>);

        return T{0.2126} * red + T{0.7152} * green + T{0.0722} * blue;
}

//

template <typename T>
constexpr numerical::Vector<3, T> xyz_to_linear_srgb(const T x, const T y, const T z)
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<3, T> rgb;

        rgb[0] = T{+3.2406255} * x + T{-1.5372080} * y + T{-0.4986286} * z;
        rgb[1] = T{-0.9689307} * x + T{+1.8757561} * y + T{+0.0415175} * z;
        rgb[2] = T{+0.0557101} * x + T{-0.2040211} * y + T{+1.0569959} * z;

        return rgb;
}

template <typename T>
constexpr numerical::Vector<3, T> linear_srgb_to_xyz(const T r, const T g, const T b)
{
        static_assert(std::is_floating_point_v<T>);

        numerical::Vector<3, T> xyz;

        xyz[0] = T{0.4124} * r + T{0.3576} * g + T{0.1805} * b;
        xyz[1] = T{0.2126} * r + T{0.7152} * g + T{0.0722} * b;
        xyz[2] = T{0.0193} * r + T{0.1192} * g + T{0.9505} * b;

        return xyz;
}
}
