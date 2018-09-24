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

#include "conversion.h"

#include "com/error.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

#define USE_COLOR_LOOKUP_TABLES 1

constexpr unsigned MAX8 = (1u << 8) - 1;
constexpr unsigned MAX16 = (1u << 16) - 1;

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
// поэтому если параметр больше 255 при unsigned char не 8 бит,
// то значение больше 1 будет далее правильно обработано
template <typename Float>
constexpr Float uint8_to_float(unsigned char c)
{
        static_assert(std::is_floating_point_v<Float>);

        ASSERT(std::numeric_limits<unsigned char>::max() == MAX8 || c <= MAX8);

        return Float(c) / Float(MAX8);
}

namespace
{
#if USE_COLOR_LOOKUP_TABLES

// clang-format off
template <typename T>
constexpr T SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE[256] =
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
constexpr unsigned char SRGB_UINT8_TO_RGB_UINT8_LOOKUP_TABLE[256] =
{
          0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,
          1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,
          4,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,
          8,   8,   8,   8,   9,   9,   9,  10,  10,  10,  11,  11,  12,  12,  12,  13,
         13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  17,  18,  18,  19,  19,  20,
         20,  21,  22,  22,  23,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,
         30,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  37,  38,  39,  40,  41,
         41,  42,  43,  44,  45,  45,  46,  47,  48,  49,  50,  51,  51,  52,  53,  54,
         55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
         71,  72,  73,  74,  76,  77,  78,  79,  80,  81,  82,  84,  85,  86,  87,  88,
         90,  91,  92,  93,  95,  96,  97,  99, 100, 101, 103, 104, 105, 107, 108, 109,
        111, 112, 114, 115, 116, 118, 119, 121, 122, 124, 125, 127, 128, 130, 131, 133,
        134, 136, 138, 139, 141, 142, 144, 146, 147, 149, 151, 152, 154, 156, 157, 159,
        161, 163, 164, 166, 168, 170, 171, 173, 175, 177, 179, 181, 183, 184, 186, 188,
        190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220,
        222, 224, 226, 229, 231, 233, 235, 237, 239, 242, 244, 246, 248, 250, 253, 255
};
// clang-format on
#endif

template <typename T>
T srgb_to_rgb(T c)
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

template <typename T>
T rgb_to_srgb(T c)
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
T rgb_luminance(T red, T green, T blue)
{
        static_assert(std::is_floating_point_v<T>);

        return T(0.2126) * red + T(0.7152) * green + T(0.0722) * blue;
}
}

namespace color_conversion
{
#if USE_COLOR_LOOKUP_TABLES

template <typename T>
T srgb_uint8_to_rgb_float(unsigned char c)
{
        if constexpr (std::numeric_limits<unsigned char>::max() == 255)
        {
                return SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE<T>[c];
        }
        else
        {
                return SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE<T>[std::min(c, static_cast<unsigned char>(255))];
        }
}

unsigned char srgb_uint8_to_rgb_uint8(unsigned char c)
{
        if constexpr (std::numeric_limits<unsigned char>::max() == 255)
        {
                return SRGB_UINT8_TO_RGB_UINT8_LOOKUP_TABLE[c];
        }
        else
        {
                return SRGB_UINT8_TO_RGB_UINT8_LOOKUP_TABLE[std::min(c, static_cast<unsigned char>(255))];
        }
}

std::uint_least16_t srgb_uint8_to_rgb_uint16(unsigned char c)
{
        return float_to_uint16(srgb_to_rgb(uint8_to_float<float>(c)));
}

#else

template <typename T>
T srgb_uint8_to_rgb_float(unsigned char c)
{
        return srgb_to_rgb(uint8_to_float<T>(c));
}

unsigned char srgb_uint8_to_rgb_uint8(unsigned char c)
{
        return float_to_uint8(srgb_to_rgb(uint8_to_float<float>(c)));
}

std::uint_least16_t srgb_uint8_to_rgb_uint16(unsigned char c)
{
        return float_to_uint16(srgb_to_rgb(uint8_to_float<float>(c)));
}

#endif

template <typename T>
T rgb_float_to_srgb_float(T c)
{
        return rgb_to_srgb<T>(c);
}

template <typename T>
unsigned char rgb_float_to_srgb_uint8(T c)
{
        return float_to_uint8(rgb_to_srgb(c));
}

template <typename T>
T rgb_float_to_rgb_luminance(T red, T green, T blue)
{
        return rgb_luminance(red, green, blue);
}

template float srgb_uint8_to_rgb_float(unsigned char c);
template double srgb_uint8_to_rgb_float(unsigned char c);
template long double srgb_uint8_to_rgb_float(unsigned char c);

template float rgb_float_to_srgb_float(float c);
template double rgb_float_to_srgb_float(double c);
template long double rgb_float_to_srgb_float(long double c);

template unsigned char rgb_float_to_srgb_uint8(float c);
template unsigned char rgb_float_to_srgb_uint8(double c);
template unsigned char rgb_float_to_srgb_uint8(long double c);

template float rgb_float_to_rgb_luminance(float red, float green, float blue);
template double rgb_float_to_rgb_luminance(double red, double green, double blue);
template long double rgb_float_to_rgb_luminance(long double red, long double green, long double blue);
}

#if !USE_COLOR_LOOKUP_TABLES
//Функции для создания таблиц
#include "com/string/str.h"
#include "com/types.h"
#include <iomanip>
#include <limits>
#include <sstream>
namespace color_conversion
{
std::string lookup_table()
{
        using T = long double;
        std::string suffix = to_upper(floating_point_suffix<T>());
        std::ostringstream oss;
        oss << std::setprecision(std::numeric_limits<T>::max_digits10);
        oss << std::scientific;
        oss << "// clang-format off\n";
        oss << "template <typename T>\n";
        oss << "constexpr T SRGB_UINT8_TO_RGB_FLOAT_LOOKUP_TABLE[256] =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i & 0b11) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << srgb_uint8_to_rgb_float<T>(i) << suffix;
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
std::string lookup_table_integer()
{
        std::ostringstream oss;
        oss << std::setfill(' ');
        oss << "// clang-format off\n";
        oss << "constexpr unsigned char SRGB_UINT8_TO_RGB_UINT8_LOOKUP_TABLE[256] =\n";
        oss << "{";
        for (int i = 0; i <= 255; ++i)
        {
                oss << ((i != 0) ? "," : "");
                oss << (((i % 16) != 0) ? " " : "\n" + std::string(8, ' '));
                oss << std::setw(3) << static_cast<int>(srgb_uint8_to_rgb_uint8(i));
        }
        oss << "\n};\n";
        oss << "// clang-format on\n";
        return oss.str();
}
}
#endif
