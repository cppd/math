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

#include "daylight_samples.h"

#include "average.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <array>

namespace ns::color
{
namespace
{
struct Component
{
        double s0;
        double s1;
        double s2;
};

// clang-format off
constexpr std::array WAVES = std::to_array<double>
({
        300, 305, 310, 315, 320, 325, 330, 335,
        340, 345, 350, 355, 360, 365, 370, 375,
        380, 385, 390, 395, 400, 405, 410, 415,
        420, 425, 430, 435, 440, 445, 450, 455,
        460, 465, 470, 475, 480, 485, 490, 495,
        500, 505, 510, 515, 520, 525, 530, 535,
        540, 545, 550, 555, 560, 565, 570, 575,
        580, 585, 590, 595, 600, 605, 610, 615,
        620, 625, 630, 635, 640, 645, 650, 655,
        660, 665, 670, 675, 680, 685, 690, 695,
        700, 705, 710, 715, 720, 725, 730, 735,
        740, 745, 750, 755, 760, 765, 770, 775,
        780, 785, 790, 795, 800, 805, 810, 815,
        820, 825, 830
});
constexpr std::array COMPONENTS = std::to_array<Component>
({
        {  0.04,   0.02,  0.00},
        {  3.02,   2.26,  1.00},
        {  6.00,   4.50,  2.00},
        { 17.80,  13.45,  3.00},
        { 29.60,  22.40,  4.00},
        { 42.45,  32.20,  6.25},
        { 55.30,  42.00,  8.50},
        { 56.30,  41.30,  8.15},
        { 57.30,  40.60,  7.80},
        { 59.55,  41.10,  7.25},
        { 61.80,  41.60,  6.70},
        { 61.65,  39.80,  6.00},
        { 61.50,  38.00,  5.30},
        { 65.15,  40.20,  5.70},
        { 68.80,  42.40,  6.10},
        { 66.10,  40.45,  4.55},
        { 63.40,  38.50,  3.00},
        { 64.60,  36.75,  2.10},
        { 65.80,  35.00,  1.20},
        { 80.30,  39.20,  0.05},
        { 94.80,  43.40, -1.10},
        { 99.80,  44.85, -0.80},
        {104.80,  46.30, -0.50},
        {105.35,  45.10, -0.60},
        {105.90,  43.90, -0.70},
        {101.35,  40.50, -0.95},
        { 96.80,  37.10, -1.20},
        {105.35,  36.90, -1.90},
        {113.90,  36.70, -2.60},
        {119.75,  36.30, -2.75},
        {125.60,  35.90, -2.90},
        {125.55,  34.25, -2.85},
        {125.50,  32.60, -2.80},
        {123.40,  30.25, -2.70},
        {121.30,  27.90, -2.60},
        {121.30,  26.10, -2.60},
        {121.30,  24.30, -2.60},
        {117.40,  22.20, -2.20},
        {113.50,  20.10, -1.80},
        {113.30,  18.15, -1.65},
        {113.10,  16.20, -1.50},
        {111.95,  14.70, -1.40},
        {110.80,  13.20, -1.30},
        {108.65,  10.90, -1.25},
        {106.50,   8.60, -1.20},
        {107.65,   7.35, -1.10},
        {108.80,   6.10, -1.00},
        {107.05,   5.15, -0.75},
        {105.30,   4.20, -0.50},
        {104.85,   3.05, -0.40},
        {104.40,   1.90, -0.30},
        {102.20,   0.95, -0.15},
        {100.00,   0.00,  0.00},
        { 98.00,  -0.80,  0.10},
        { 96.00,  -1.60,  0.20},
        { 95.55,  -2.55,  0.35},
        { 95.10,  -3.50,  0.50},
        { 92.10,  -3.50,  1.30},
        { 89.10,  -3.50,  2.10},
        { 89.80,  -4.65,  2.65},
        { 90.50,  -5.80,  3.20},
        { 90.40,  -6.50,  3.65},
        { 90.30,  -7.20,  4.10},
        { 89.35,  -7.90,  4.40},
        { 88.40,  -8.60,  4.70},
        { 86.20,  -9.05,  4.90},
        { 84.00,  -9.50,  5.10},
        { 84.55, -10.20,  5.90},
        { 85.10, -10.90,  6.70},
        { 83.50, -10.80,  7.00},
        { 81.90, -10.70,  7.30},
        { 82.25, -11.35,  7.95},
        { 82.60, -12.00,  8.60},
        { 83.75, -13.00,  9.20},
        { 84.90, -14.00,  9.80},
        { 83.10, -13.80, 10.00},
        { 81.30, -13.60, 10.20},
        { 76.60, -12.80,  9.25},
        { 71.90, -12.00,  8.30},
        { 73.10, -12.65,  8.95},
        { 74.30, -13.30,  9.60},
        { 75.35, -13.10,  9.05},
        { 76.40, -12.90,  8.50},
        { 69.85, -11.75,  7.75},
        { 63.30, -10.60,  7.00},
        { 67.50, -11.10,  7.30},
        { 71.70, -11.60,  7.60},
        { 74.35, -11.90,  7.80},
        { 77.00, -12.20,  8.00},
        { 71.10, -11.20,  7.35},
        { 65.20, -10.20,  6.70},
        { 56.45,  -9.00,  5.95},
        { 47.70,  -7.80,  5.20},
        { 58.15,  -9.50,  6.30},
        { 68.60, -11.20,  7.40},
        { 66.80, -10.80,  7.10},
        { 65.00, -10.40,  6.80},
        { 65.50, -10.50,  6.90},
        { 66.00, -10.60,  7.00},
        { 63.50, -10.15,  6.70},
        { 61.00,  -9.70,  6.40},
        { 57.15,  -9.00,  5.95},
        { 53.30,  -8.30,  5.50},
        { 56.10,  -8.80,  5.80},
        { 58.90,  -9.30,  6.10},
        { 60.40,  -9.55,  6.30},
        { 61.90,  -9.80,  6.50}
});
constexpr std::array D65 = std::to_array<double>
({
          0.0341,   1.6643,   3.2945,  11.7652,
         20.2360,  28.6447,  37.0535,  38.5011,
         39.9488,  42.4302,  44.9117,  45.7750,
         46.6383,  49.3637,  52.0891,  51.0323,
         49.9755,  52.3118,  54.6482,  68.7015,
         82.7549,  87.1204,  91.4860,  92.4589,
         93.4318,  90.0570,  86.6823,  95.7736,
        104.8650, 110.9360, 117.0080, 117.4100,
        117.8120, 116.3360, 114.8610, 115.3920,
        115.9230, 112.3670, 108.8110, 109.0820,
        109.3540, 108.5780, 107.8020, 106.2960,
        104.7900, 106.2390, 107.6890, 106.0470,
        104.4050, 104.2250, 104.0460, 102.0230,
        100.0000,  98.1671,  96.3342,  96.0611,
         95.7880,  92.2368,  88.6856,  89.3459,
         90.0062,  89.8026,  89.5991,  88.6489,
         87.6987,  85.4936,  83.2886,  83.4939,
         83.6992,  81.8630,  80.0268,  80.1207,
         80.2146,  81.2462,  82.2778,  80.2810,
         78.2842,  74.0027,  69.7213,  70.6652,
         71.6091,  72.9790,  74.3490,  67.9765,
         61.6040,  65.7448,  69.8856,  72.4863,
         75.0870,  69.3398,  63.5927,  55.0054,
         46.4182,  56.6118,  66.8054,  65.0941,
         63.3828,  63.8434,  64.3040,  61.8779,
         59.4519,  55.7054,  51.9590,  54.6998,
         57.4406,  58.8765,  60.3125
});
// clang-format on

static_assert(std::is_sorted(std::cbegin(WAVES), std::cend(WAVES)));
static_assert(WAVES.front() == DAYLIGHT_SAMPLES_MIN_WAVELENGTH);
static_assert(WAVES.back() == DAYLIGHT_SAMPLES_MAX_WAVELENGTH);
static_assert(WAVES.size() == COMPONENTS.size());
static_assert(WAVES.size() == D65.size());
}

std::vector<double> daylight_d65_samples(const int from, const int to, const int count)
{
        return average<double>(WAVES, D65, from, to, count);
}

std::vector<double> daylight_samples(const double cct, const int from, const int to, const int count)
{
        const double xd = [&cct]
        {
                if (cct >= 4000 && cct <= 7000)
                {
                        const double t1 = 1e3 / cct;
                        const double t2 = 1e6 / (cct * cct);
                        const double t3 = 1e9 / (cct * cct * cct);
                        return 0.244063 + 0.09911 * t1 + 2.9678 * t2 - 4.607 * t3;
                }
                if (cct > 7000 && cct <= 25000)
                {
                        const double t1 = 1e3 / cct;
                        const double t2 = 1e6 / (cct * cct);
                        const double t3 = 1e9 / (cct * cct * cct);
                        return 0.23704 + 0.24748 * t1 + 1.9018 * t2 - 2.0064 * t3;
                }
                error("Unsupported CCT " + to_string(cct));
        }();

        const double yd = xd * (-3.0 * xd + 2.87) - 0.275;

        const double m = 0.0241 + 0.2562 * xd - 0.7341 * yd;
        const double m1 = (-1.3515 - 1.7703 * xd + 5.9114 * yd) / m;
        const double m2 = (0.03 - 31.4424 * xd + 30.0717 * yd) / m;

        std::array<double, WAVES.size()> s;
        for (std::size_t i = 0; i < WAVES.size(); ++i)
        {
                s[i] = COMPONENTS[i].s0 + m1 * COMPONENTS[i].s1 + m2 * COMPONENTS[i].s2;
        }

        return average<double>(WAVES, s, from, to, count);
}
}
