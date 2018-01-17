/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "colors.h"

#include <cmath>

namespace
{
#if 1

// clang-format off
constexpr float FROM_SRGB_TO_RGB_LOOKUP_TABLE[256] =
{
        0.000000000e+00f, 3.035269835e-04f, 6.070539671e-04f, 9.105809506e-04f,
        1.214107934e-03f, 1.517634918e-03f, 1.821161901e-03f, 2.124688885e-03f,
        2.428215868e-03f, 2.731742852e-03f, 3.035269835e-03f, 3.346535764e-03f,
        3.676507324e-03f, 4.024717018e-03f, 4.391442037e-03f, 4.776953481e-03f,
        5.181516702e-03f, 5.605391624e-03f, 6.048833023e-03f, 6.512090793e-03f,
        6.995410187e-03f, 7.499032043e-03f, 8.023192985e-03f, 8.568125618e-03f,
        9.134058702e-03f, 9.721217320e-03f, 1.032982303e-02f, 1.096009401e-02f,
        1.161224518e-02f, 1.228648836e-02f, 1.298303234e-02f, 1.370208305e-02f,
        1.444384360e-02f, 1.520851442e-02f, 1.599629337e-02f, 1.680737575e-02f,
        1.764195449e-02f, 1.850022013e-02f, 1.938236096e-02f, 2.028856306e-02f,
        2.121901038e-02f, 2.217388479e-02f, 2.315336618e-02f, 2.415763245e-02f,
        2.518685963e-02f, 2.624122189e-02f, 2.732089164e-02f, 2.842603950e-02f,
        2.955683444e-02f, 3.071344373e-02f, 3.189603307e-02f, 3.310476657e-02f,
        3.433980681e-02f, 3.560131488e-02f, 3.688945040e-02f, 3.820437160e-02f,
        3.954623528e-02f, 4.091519691e-02f, 4.231141062e-02f, 4.373502926e-02f,
        4.518620439e-02f, 4.666508634e-02f, 4.817182423e-02f, 4.970656598e-02f,
        5.126945837e-02f, 5.286064702e-02f, 5.448027644e-02f, 5.612849005e-02f,
        5.780543019e-02f, 5.951123816e-02f, 6.124605423e-02f, 6.301001765e-02f,
        6.480326669e-02f, 6.662593864e-02f, 6.847816984e-02f, 7.036009570e-02f,
        7.227185068e-02f, 7.421356838e-02f, 7.618538148e-02f, 7.818742181e-02f,
        8.021982031e-02f, 8.228270713e-02f, 8.437621154e-02f, 8.650046204e-02f,
        8.865558629e-02f, 9.084171118e-02f, 9.305896285e-02f, 9.530746663e-02f,
        9.758734714e-02f, 9.989872825e-02f, 1.022417331e-01f, 1.046164841e-01f,
        1.070231030e-01f, 1.094617108e-01f, 1.119324278e-01f, 1.144353738e-01f,
        1.169706678e-01f, 1.195384280e-01f, 1.221387722e-01f, 1.247718176e-01f,
        1.274376804e-01f, 1.301364767e-01f, 1.328683216e-01f, 1.356333297e-01f,
        1.384316150e-01f, 1.412632911e-01f, 1.441284709e-01f, 1.470272665e-01f,
        1.499597898e-01f, 1.529261520e-01f, 1.559264637e-01f, 1.589608351e-01f,
        1.620293756e-01f, 1.651321945e-01f, 1.682694002e-01f, 1.714411007e-01f,
        1.746474037e-01f, 1.778884160e-01f, 1.811642442e-01f, 1.844749945e-01f,
        1.878207723e-01f, 1.912016827e-01f, 1.946178304e-01f, 1.980693196e-01f,
        2.015562538e-01f, 2.050787364e-01f, 2.086368701e-01f, 2.122307574e-01f,
        2.158605001e-01f, 2.195261997e-01f, 2.232279573e-01f, 2.269658735e-01f,
        2.307400485e-01f, 2.345505822e-01f, 2.383975738e-01f, 2.422811225e-01f,
        2.462013267e-01f, 2.501582847e-01f, 2.541520943e-01f, 2.581828529e-01f,
        2.622506575e-01f, 2.663556048e-01f, 2.704977910e-01f, 2.746773121e-01f,
        2.788942635e-01f, 2.831487404e-01f, 2.874408377e-01f, 2.917706498e-01f,
        2.961382708e-01f, 3.005437944e-01f, 3.049873141e-01f, 3.094689228e-01f,
        3.139887134e-01f, 3.185467781e-01f, 3.231432091e-01f, 3.277780981e-01f,
        3.324515363e-01f, 3.371636150e-01f, 3.419144249e-01f, 3.467040564e-01f,
        3.515325995e-01f, 3.564001441e-01f, 3.613067798e-01f, 3.662525956e-01f,
        3.712376805e-01f, 3.762621230e-01f, 3.813260114e-01f, 3.864294338e-01f,
        3.915724777e-01f, 3.967552307e-01f, 4.019777798e-01f, 4.072402119e-01f,
        4.125426135e-01f, 4.178850708e-01f, 4.232676700e-01f, 4.286904966e-01f,
        4.341536362e-01f, 4.396571738e-01f, 4.452011945e-01f, 4.507857828e-01f,
        4.564110232e-01f, 4.620769997e-01f, 4.677837961e-01f, 4.735314961e-01f,
        4.793201831e-01f, 4.851499401e-01f, 4.910208498e-01f, 4.969329951e-01f,
        5.028864580e-01f, 5.088813209e-01f, 5.149176654e-01f, 5.209955732e-01f,
        5.271151257e-01f, 5.332764040e-01f, 5.394794890e-01f, 5.457244614e-01f,
        5.520114015e-01f, 5.583403896e-01f, 5.647115057e-01f, 5.711248295e-01f,
        5.775804404e-01f, 5.840784179e-01f, 5.906188409e-01f, 5.972017884e-01f,
        6.038273389e-01f, 6.104955708e-01f, 6.172065624e-01f, 6.239603917e-01f,
        6.307571363e-01f, 6.375968740e-01f, 6.444796820e-01f, 6.514056374e-01f,
        6.583748173e-01f, 6.653872983e-01f, 6.724431570e-01f, 6.795424696e-01f,
        6.866853124e-01f, 6.938717613e-01f, 7.011018919e-01f, 7.083757799e-01f,
        7.156935005e-01f, 7.230551289e-01f, 7.304607401e-01f, 7.379104088e-01f,
        7.454042095e-01f, 7.529422168e-01f, 7.605245047e-01f, 7.681511472e-01f,
        7.758222183e-01f, 7.835377915e-01f, 7.912979403e-01f, 7.991027380e-01f,
        8.069522577e-01f, 8.148465722e-01f, 8.227857544e-01f, 8.307698768e-01f,
        8.387990117e-01f, 8.468732315e-01f, 8.549926081e-01f, 8.631572135e-01f,
        8.713671192e-01f, 8.796223969e-01f, 8.879231179e-01f, 8.962693534e-01f,
        9.046611744e-01f, 9.130986518e-01f, 9.215818563e-01f, 9.301108584e-01f,
        9.386857285e-01f, 9.473065367e-01f, 9.559733532e-01f, 9.646862479e-01f,
        9.734452904e-01f, 9.822505503e-01f, 9.911020971e-01f, 1.000000000e+00f
};
// clang-format on

double srgb_integer_to_rgb_float(unsigned char c)
{
        static_assert(std::numeric_limits<unsigned char>::digits == 8);

        return FROM_SRGB_TO_RGB_LOOKUP_TABLE[c];
}

#else

double srgb_to_rgb(double c)
{
        if (c > 1.0)
        {
                return 1.0;
        }
        if (c >= 0.04045)
        {
                return std::pow((c + 0.055) / 1.055, 2.4);
        }
        if (c >= 0.0)
        {
                return c / 12.92;
        }
        return 0.0;
}

double srgb_integer_to_rgb_float(unsigned char c)
{
        return srgb_to_rgb(c / 255.0);
}

#endif

double rgb_to_srgb(double c)
{
        if (c > 1.0)
        {
                return 1.0;
        }
        if (c >= 0.0031308)
        {
                return 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
        }
        if (c >= 0.0)
        {
                return c * 12.92;
        }
        return 0.0;
}

unsigned char rgb_float_to_srgb_integer(double c)
{
        return static_cast<unsigned char>(rgb_to_srgb(c) * 255.0 + 0.5);
}
}

vec3 srgb_integer_to_rgb_float(unsigned char r, unsigned char g, unsigned char b)
{
        return vec3(srgb_integer_to_rgb_float(r), srgb_integer_to_rgb_float(g), srgb_integer_to_rgb_float(b));
}

std::array<unsigned char, 3> rgb_float_to_srgb_integer(const vec3& c)
{
        return {{rgb_float_to_srgb_integer(c[0]), rgb_float_to_srgb_integer(c[1]), rgb_float_to_srgb_integer(c[2])}};
}

double luminance_of_rgb(const vec3& c)
{
        return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
}

#if 0
//Функция для создания таблицы
#include <limits>
#include <sstream>
std::string lookup_table()
{
        std::ostringstream oss;
        oss << std::setprecision(std::numeric_limits<float>::max_digits10);
        oss << std::scientific;
        oss << "// clang-format off\n";
        oss << "constexpr float FROM_SRGB_TO_RGB_LOOKUP_TABLE[256] =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i & 0b11) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << srgb_to_rgb(i / 255.0) << "f";
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
#endif
