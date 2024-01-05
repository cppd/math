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

#include <src/color/color.h>
#include <src/color/illuminants.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/test/test.h>

#include <string>

namespace ns::color
{
namespace
{
void compare(const Spectrum& s, const RGB<float>& c, const char* const text)
{
        const auto rgb = to_color<RGB<float>>(s);
        if (!rgb.equal_to_relative(c, 1e-4))
        {
                error(std::string(text) + " " + to_string(rgb) + " is not equal to " + to_string(c));
        }
}

void test()
{
        LOG("Test illuminants");

        compare(daylight_d65(), {0.9978, 1.0008, 0.9985}, "Daylight D65");
        compare(daylight(5000), {1.1708, 0.9775, 0.7202}, "Daylight 5000K");
        compare(daylight(10000), {0.8254, 1.0112, 1.4027}, "Daylight 10000K");
        compare(blackbody_a(), {1.8248, 0.8323, 0.2321}, "Blackbody A");
        compare(blackbody(5000), {1.2074, 0.9625, 0.761}, "Blackbody 5000K");
        compare(blackbody(10000), {0.8718, 0.9948, 1.4288}, "Blackbody 10000K");

        LOG("Test illuminants passed");
}

TEST_SMALL("Illuminants", test)
}
}
