/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "conversion.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>

#define USE_COLOR_LOOKUP_TABLES

namespace
{
constexpr unsigned char MAX8 = (1u << 8) - 1;
constexpr std::uint_least16_t MAX16 = (1u << 16) - 1;

// Функции работают после преобразований с плавающей точкой,
// поэтому параметр функций находится в интервале [0, 1]
template <typename Float>
constexpr unsigned char float_to_uint8(Float c)
{
        static_assert(std::is_floating_point_v<Float>);

        ASSERT(c >= 0 && c <= 1);

        return c * Float(MAX8) + Float(0.5);
}
template <typename Float>
constexpr std::uint_least16_t float_to_uint16(Float c)
{
        static_assert(std::is_floating_point_v<Float>);

        ASSERT(c >= 0 && c <= 1);

        return c * Float(MAX16) + Float(0.5);
}

// Функция работает до преобразований с плавающей точкой,
// поэтому если параметр больше MAX8 при unsigned char не 8 бит,
// то значение больше 1 будет далее правильно обработано
template <typename Float, typename UInt8>
constexpr Float uint8_to_float(UInt8 c)
{
        static_assert(std::is_same_v<UInt8, unsigned char>);
        static_assert(std::is_floating_point_v<Float>);

        ASSERT(limits<unsigned char>::max() == MAX8 || c <= MAX8);

        return Float(c) / Float(MAX8);
}

#if defined(USE_COLOR_LOOKUP_TABLES)

// clang-format off
template <typename T>
constexpr std::array<T, 256> SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE =
{
        0.000000000000000000000e+00L, 3.035269835488374933221e-04L, 6.070539670976749866442e-04L, 9.105809506465124800192e-04L,
        1.214107934195349973288e-03L, 1.517634917744187466610e-03L, 1.821161901293024960038e-03L, 2.124688884841862453466e-03L,
        2.428215868390699946577e-03L, 2.731742851939537439899e-03L, 3.035269835488374933221e-03L, 3.346535763899159701740e-03L,
        3.676507324047436250104e-03L, 4.024717018496305703786e-03L, 4.391442037410294362864e-03L, 4.776953480693728742560e-03L,
        5.181516702338386918977e-03L, 5.605391624202722681259e-03L, 6.048833022857054623069e-03L, 6.512090792594473683855e-03L,
        6.995410187265387874825e-03L, 7.499032043226172490917e-03L, 8.023192985384995986786e-03L, 8.568125618069304729133e-03L,
        9.134058702220789062427e-03L, 9.721217320237846692000e-03L, 1.032982302962694028285e-02L, 1.096009400648824435342e-02L,
        1.161224517974388505224e-02L, 1.228648835691587084055e-02L, 1.298303234217301197175e-02L, 1.370208304728968735728e-02L,
        1.444384359609254527088e-02L, 1.520851442291271006541e-02L, 1.599629336550963283536e-02L, 1.680737575288738151130e-02L,
        1.764195448838408212447e-02L, 1.850022012837969387930e-02L, 1.938236095693572819910e-02L, 2.028856305665239681858e-02L,
        2.121901037600356125583e-02L, 2.217388479338738207239e-02L, 2.315336617811041106959e-02L, 2.415763244850475524253e-02L,
        2.518685962736162951214e-02L, 2.624122189484989761741e-02L, 2.732089163907489697962e-02L, 2.842603950442079611742e-02L,
        2.955683443780880517082e-02L, 3.071344373299363014526e-02L, 3.189603307301152462105e-02L, 3.310476657088505491919e-02L,
        3.433980680868217489978e-02L, 3.560131487502033128772e-02L, 3.688945040110002620648e-02L, 3.820437159534649299415e-02L,
        3.954623527673284391143e-02L, 4.091519690685317913167e-02L, 4.231141062080966491429e-02L, 4.373502925697345946219e-02L,
        4.518620438567555543287e-02L, 4.666508633688008942386e-02L, 4.817182422688941560764e-02L, 4.970656598412722908248e-02L,
        5.126945837404323443024e-02L, 5.286064702318026650339e-02L, 5.448027644244236749921e-02L, 5.612849004960009094957e-02L,
        5.780543019106722560816e-02L, 5.951123816298119766183e-02L, 6.124605423161760679287e-02L, 6.301001765316766992016e-02L,
        6.480326669290577642395e-02L, 6.662593864377289174704e-02L, 6.847816984440017453270e-02L, 7.036009569659588876742e-02L,
        7.227185068231748976935e-02L, 7.421356838014963640065e-02L, 7.618538148130782442928e-02L, 7.818742180518634387340e-02L,
        8.021982031446833113364e-02L, 8.228270712981481068343e-02L, 8.437621154414879689061e-02L, 8.650046203654975106767e-02L,
        8.865558628577295809613e-02L, 9.084171118340769874569e-02L, 9.305896284668744422298e-02L, 9.530746663096468708650e-02L,
        9.758734714186244412718e-02L, 9.989872824711392009867e-02L, 1.022417330881013050533e-01L, 1.046164840911041891053e-01L,
        1.070231029782676162888e-01L, 1.094617107782993612605e-01L, 1.119324278369055981831e-01L, 1.144353738269737377680e-01L,
        1.169706677585108359527e-01L, 1.195384279883456238522e-01L, 1.221387722296018733253e-01L, 1.247718175609504930528e-01L,
        1.274376804356474479510e-01L, 1.301364766903643059592e-01L, 1.328683215538179432261e-01L, 1.356333296552056775673e-01L,
        1.384316150324518525182e-01L, 1.412632911402716578194e-01L, 1.441284708580577474785e-01L, 1.470272664975950021389e-01L,
        1.499597898106085777051e-01L, 1.529261519961501878192e-01L, 1.559264637078273809881e-01L, 1.589608350608803962183e-01L,
        1.620293756391110108928e-01L, 1.651321945016676331290e-01L, 1.682694001896907351956e-01L, 1.714411007328225774657e-01L,
        1.746474036555850302985e-01L, 1.778884159836291657171e-01L, 1.811642442498601614188e-01L, 1.844749945004409353764e-01L,
        1.878207723006778106096e-01L, 1.912016827407913955141e-01L, 1.946178304415757566442e-01L, 1.980693195599488554647e-01L,
        2.015562537943971209453e-01L, 2.050787363903169335843e-01L, 2.086368701452557035690e-01L, 2.122307574140551380577e-01L,
        2.158605001138992068193e-01L, 2.195261997292692340985e-01L, 2.232279573168084659831e-01L, 2.269658735100983873055e-01L,
        2.307400485243489893618e-01L, 2.345505821610051203150e-01L, 2.383975738122709829689e-01L, 2.422811224655547800762e-01L,
        2.462013267078354456538e-01L, 2.501582847299533404121e-01L, 2.541520943308267327344e-01L, 2.581828529215958305818e-01L,
        2.622506575296960767431e-01L, 2.663556048028623683709e-01L, 2.704977910130658124170e-01L, 2.746773120603845802241e-01L,
        2.788942634768103795363e-01L, 2.831487404299920172238e-01L, 2.874408377269174822680e-01L, 2.917706498175359396889e-01L,
        2.961382707983209837800e-01L, 3.005437944157764616694e-01L, 3.049873140698861412258e-01L, 3.094689228175084608717e-01L,
        3.139887133757175645247e-01L, 3.185467781250917916017e-01L, 3.231432091129507591979e-01L, 3.277780980565421425162e-01L,
        3.324515363461792298344e-01L, 3.371636150483302978866e-01L, 3.419144249086608268343e-01L, 3.467040563550295452478e-01L,
        3.515325995004392699284e-01L, 3.564001441459434795986e-01L, 3.613067797835095363895e-01L, 3.662525955988394455527e-01L,
        3.712376804741490202708e-01L, 3.762621229909062960518e-01L, 3.813260114325300174270e-01L, 3.864294337870489986651e-01L,
        3.915724777497231392381e-01L, 3.967552307256268558783e-01L, 4.019777798321956732296e-01L, 4.072402119017366959024e-01L,
        4.125426134839036687238e-01L, 4.178850708481373115908e-01L, 4.232676699860716011876e-01L, 4.286904966139066531226e-01L,
        4.341536361747488432974e-01L, 4.396571738409187916268e-01L, 4.452011945162278157761e-01L, 4.507857828382234482479e-01L,
        4.564110231804045960779e-01L, 4.620769996544069082548e-01L, 4.677837961121589029664e-01L, 4.735314961480093933854e-01L,
        4.793201831008267387805e-01L, 4.851499400560704343779e-01L, 4.910208498478355425277e-01L, 4.969329950608704555594e-01L,
        5.028864580325684695717e-01L, 5.088813208549336376124e-01L, 5.149176653765213596469e-01L, 5.209955732043541569822e-01L,
        5.271151257058130679438e-01L, 5.332764040105050926599e-01L, 5.394794890121071050202e-01L, 5.457244613701866394698e-01L,
        5.520114015119999534135e-01L, 5.583403896342677554345e-01L, 5.647115057049289819978e-01L, 5.711248294648729958187e-01L,
        5.775804404296505730095e-01L, 5.840784178911640359762e-01L, 5.906188409193368823178e-01L, 5.972017883637632535278e-01L,
        6.038273388553375779753e-01L, 6.104955708078647171663e-01L, 6.172065624196509364251e-01L, 6.239603916750760149589e-01L,
        6.307571363461468037575e-01L, 6.375968739940325323611e-01L, 6.444796819705821607502e-01L, 6.514056374198240653012e-01L,
        6.583748172794483428114e-01L, 6.653872982822720099890e-01L, 6.724431569576873702157e-01L, 6.795424696330938147846e-01L,
        6.866853124353133194725e-01L, 6.938717612919898923172e-01L, 7.011018919329732238107e-01L, 7.083757798916867844835e-01L,
        7.156935005064806126860e-01L, 7.230551289219690262147e-01L, 7.304607400903534919143e-01L, 7.379104087727308788252e-01L,
        7.454042095403873175721e-01L, 7.529422167760778855433e-01L, 7.605245046752923303106e-01L, 7.681511472475070425475e-01L,
        7.758222183174234838458e-01L, 7.835377915261932717456e-01L, 7.912979403326301202216e-01L, 7.991027380144088296471e-01L,
        8.069522576692515177573e-01L, 8.148465722161012783674e-01L, 8.227857543962834518340e-01L, 8.307698767746546871828e-01L,
        8.387990117407399734959e-01L, 8.468732315098578138676e-01L, 8.549926081242337124735e-01L, 8.631572134541021422633e-01L,
        8.713671191987971577494e-01L, 8.796223968878318141677e-01L, 8.879231178819665502725e-01L, 8.962693533742666941993e-01L,
        9.046611743911492395744e-01L, 9.130986517934190457749e-01L, 9.215818562772946101686e-01L, 9.301108583754235543668e-01L,
        9.386857284578879684692e-01L, 9.473065367331997523514e-01L, 9.559733532492860923939e-01L, 9.646862478944652077124e-01L,
        9.734452903984124977300e-01L, 9.822505503331172218998e-01L, 9.911020971138298405433e-01L, 1.000000000000000000000e+00L
};
// clang-format on

// clang-format off
constexpr std::array<std::uint_least16_t, 256> SRGB_UINT8_TO_RGB_UINT16_LOOKUP_TABLE =
{
            0,    20,    40,    60,    80,    99,   119,   139,   159,   179,   199,   219,   241,   264,   288,   313,
          340,   367,   396,   427,   458,   491,   526,   562,   599,   637,   677,   718,   761,   805,   851,   898,
          947,   997,  1048,  1101,  1156,  1212,  1270,  1330,  1391,  1453,  1517,  1583,  1651,  1720,  1790,  1863,
         1937,  2013,  2090,  2170,  2250,  2333,  2418,  2504,  2592,  2681,  2773,  2866,  2961,  3058,  3157,  3258,
         3360,  3464,  3570,  3678,  3788,  3900,  4014,  4129,  4247,  4366,  4488,  4611,  4736,  4864,  4993,  5124,
         5257,  5392,  5530,  5669,  5810,  5953,  6099,  6246,  6395,  6547,  6700,  6856,  7014,  7174,  7335,  7500,
         7666,  7834,  8004,  8177,  8352,  8528,  8708,  8889,  9072,  9258,  9445,  9635,  9828, 10022, 10219, 10418,
        10619, 10822, 11028, 11235, 11446, 11658, 11873, 12090, 12309, 12530, 12754, 12980, 13209, 13440, 13673, 13909,
        14146, 14387, 14629, 14874, 15122, 15371, 15623, 15878, 16135, 16394, 16656, 16920, 17187, 17456, 17727, 18001,
        18277, 18556, 18837, 19121, 19407, 19696, 19987, 20281, 20577, 20876, 21177, 21481, 21787, 22096, 22407, 22721,
        23038, 23357, 23678, 24002, 24329, 24658, 24990, 25325, 25662, 26001, 26344, 26688, 27036, 27386, 27739, 28094,
        28452, 28813, 29176, 29542, 29911, 30282, 30656, 31033, 31412, 31794, 32179, 32567, 32957, 33350, 33745, 34143,
        34544, 34948, 35355, 35764, 36176, 36591, 37008, 37429, 37852, 38278, 38706, 39138, 39572, 40009, 40449, 40891,
        41337, 41785, 42236, 42690, 43147, 43606, 44069, 44534, 45002, 45473, 45947, 46423, 46903, 47385, 47871, 48359,
        48850, 49344, 49841, 50341, 50844, 51349, 51858, 52369, 52884, 53401, 53921, 54445, 54971, 55500, 56032, 56567,
        57105, 57646, 58190, 58737, 59287, 59840, 60396, 60955, 61517, 62082, 62650, 63221, 63795, 64372, 64952, 65535
};
// clang-format on

#else

template <typename T>
T srgb_to_linear(T c)
{
        static_assert(std::is_floating_point_v<T>);

        if (c >= 1)
        {
                return 1;
        }
        if (c >= T(0.04045))
        {
                return std::pow((c + T(0.055)) / T(1.055), T(2.4));
        }
        if (c > 0)
        {
                return c / T(12.92);
        }
        return 0;
}

#endif

template <typename T>
T linear_to_srgb(T c)
{
        static_assert(std::is_floating_point_v<T>);

        if (c >= 1)
        {
                return 1;
        }
        if (c >= T(0.0031308))
        {
                return T(1.055) * std::pow(c, T(1) / T(2.4)) - T(0.055);
        }
        if (c > 0)
        {
                return c * T(12.92);
        }
        return 0;
}

template <typename T>
T linear_luminance(T red, T green, T blue)
{
        static_assert(std::is_floating_point_v<T>);

        return T(0.2126) * red + T(0.7152) * green + T(0.0722) * blue;
}
}

namespace color_conversion
{
#if defined(USE_COLOR_LOOKUP_TABLES)

template <typename T, typename UInt8>
T srgb_uint8_to_linear_float(UInt8 c)
{
        static_assert(std::is_same_v<UInt8, unsigned char>);
        static_assert(std::is_floating_point_v<T>);

        if constexpr (limits<unsigned char>::max() == MAX8)
        {
                return SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE<T>[c];
        }
        else
        {
                return SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE<T>[std::min(c, MAX8)];
        }
}

template <typename UInt8>
std::uint_least16_t srgb_uint8_to_linear_uint16(UInt8 c)
{
        static_assert(std::is_same_v<UInt8, unsigned char>);

        if constexpr (limits<unsigned char>::max() == MAX8)
        {
                return SRGB_UINT8_TO_RGB_UINT16_LOOKUP_TABLE[c];
        }
        else
        {
                return SRGB_UINT8_TO_RGB_UINT16_LOOKUP_TABLE[std::min(c, MAX8)];
        }
}

#else

template <typename T, typename UInt8>
T srgb_uint8_to_linear_float(UInt8 c)
{
        return srgb_to_linear(uint8_to_float<T>(c));
}

template <typename UInt8>
std::uint_least16_t srgb_uint8_to_linear_uint16(UInt8 c)
{
        return float_to_uint16(srgb_to_linear(uint8_to_float<float>(c)));
}

#endif

template <typename T, typename UInt8>
T linear_uint8_to_linear_float(UInt8 c)
{
        static_assert(std::is_same_v<UInt8, unsigned char>);
        if constexpr (limits<unsigned char>::max() != MAX8)
        {
                c = std::min(c, MAX8);
        }
        return uint8_to_float<T>(c);
}

template <typename UInt8>
std::uint_least16_t linear_uint8_to_linear_uint16(UInt8 c)
{
        static_assert(std::is_same_v<UInt8, unsigned char>);
        if constexpr (limits<unsigned char>::max() != MAX8)
        {
                c = std::min(c, MAX8);
        }
        return float_to_uint16(uint8_to_float<float>(c));
}

//

template <typename T>
T linear_float_to_srgb_float(T c)
{
        return linear_to_srgb<T>(c);
}

//

template <typename T>
unsigned char linear_float_to_srgb_uint8(T c)
{
        return float_to_uint8(linear_to_srgb(c));
}

template <typename T>
unsigned char linear_float_to_linear_uint8(T c)
{
        return float_to_uint8(c);
}

template <typename T>
std::uint_least16_t linear_float_to_linear_uint16(T c)
{
        return float_to_uint16(c);
}

//

template <typename T>
T linear_float_to_linear_luminance(T red, T green, T blue)
{
        return linear_luminance(red, green, blue);
}

//

template float srgb_uint8_to_linear_float(unsigned char c);
template double srgb_uint8_to_linear_float(unsigned char c);
template long double srgb_uint8_to_linear_float(unsigned char c);

template std::uint_least16_t srgb_uint8_to_linear_uint16(unsigned char c);

template float linear_uint8_to_linear_float(unsigned char c);
template double linear_uint8_to_linear_float(unsigned char c);
template long double linear_uint8_to_linear_float(unsigned char c);

template std::uint_least16_t linear_uint8_to_linear_uint16(unsigned char c);

//

template float linear_float_to_srgb_float(float c);
template double linear_float_to_srgb_float(double c);
template long double linear_float_to_srgb_float(long double c);

//

template unsigned char linear_float_to_srgb_uint8(float c);
template unsigned char linear_float_to_srgb_uint8(double c);
template unsigned char linear_float_to_srgb_uint8(long double c);

template unsigned char linear_float_to_linear_uint8(float c);
template unsigned char linear_float_to_linear_uint8(double c);
template unsigned char linear_float_to_linear_uint8(long double c);

template std::uint_least16_t linear_float_to_linear_uint16(float c);
template std::uint_least16_t linear_float_to_linear_uint16(double c);
template std::uint_least16_t linear_float_to_linear_uint16(long double c);

//

template float linear_float_to_linear_luminance(float red, float green, float blue);
template double linear_float_to_linear_luminance(double red, double green, double blue);
template long double linear_float_to_linear_luminance(long double red, long double green, long double blue);
}

#if !defined(USE_COLOR_LOOKUP_TABLES)
//Функции для создания таблиц
#include <src/util/string/str.h>
#include <src/com/type/name.h>
#include <src/com/type/limit.h>
#include <iomanip>
#include <sstream>
namespace color_conversion
{
std::string lookup_table_float()
{
        using T = long double;
        std::string suffix = to_upper(floating_point_suffix<T>());
        std::ostringstream oss;
        oss << std::setprecision(limits<T>::max_digits10);
        oss << std::scientific;
        oss << "// clang-format off\n";
        oss << "template <typename T>\n";
        oss << "constexpr T SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE[256] =\n";
        oss << "{";
        for (unsigned i = 0; i <= MAX8; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i & 0b11) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << srgb_uint8_to_linear_float<T>(static_cast<unsigned char>(i)) << suffix;
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
std::string lookup_table_uint16()
{
        std::ostringstream oss;
        oss << std::setfill(' ');
        oss << "// clang-format off\n";
        oss << "constexpr std::uint_least16_t SRGB_UINT8_TO_RGB_UINT16_LOOKUP_TABLE[256] =\n";
        oss << "{";
        for (unsigned i = 0; i <= MAX8; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i % 16) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << std::setw(5)
                    << static_cast<unsigned>(srgb_uint8_to_linear_uint16(static_cast<unsigned char>(i)));
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
}
#endif
