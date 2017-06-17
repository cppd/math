/*
Copyright (C) 2017 Topological Manifold

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

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

constexpr int LINE_LENGTH = 12;

namespace
{
[[noreturn]] void error(const std::string& msg)
{
        std::cerr << msg << std::endl;
        std::exit(EXIT_FAILURE);
}

int to_int(char c)
{
        return static_cast<unsigned char>(c);
}
}

int main(int argc, char* argv[])
{
        if (argc != 4)
        {
                error("Usage: program \"type\" \"file_in\" \"file_out\"");
        }

        std::string type = argv[1];

        std::ifstream ifs(argv[2], type == "bin" ? (std::ios_base::in | std::ios_base::binary) : std::ios_base::in);
        if (!ifs)
        {
                error("error open file " + std::string(argv[2]));
        }

        std::ofstream ofs(argv[3]);
        if (!ofs)
        {
                error("error open file " + std::string(argv[3]));
        }

        long long i = 0;

        ofs << std::hex << std::setfill('0');

        if (type == "bin")
        {
                for (char c; ifs.get(c); ++i)
                {
                        if (i != 0)
                        {
                                ofs << ',';
                                if (i % LINE_LENGTH != 0)
                                {
                                        ofs << ' ';
                                }
                                else
                                {
                                        ofs << '\n';
                                }
                        }
                        ofs << "0x" << std::setw(2) << to_int(c);
                }
        }
        else if (type == "str")
        {
                ofs << "\"";
                for (char c; ifs.get(c); ++i)
                {
                        if (i != 0)
                        {
                                if (i % LINE_LENGTH == 0)
                                {
                                        ofs << "\"\n\"";
                                }
                        }
                        ofs << "\\x" << std::setw(2) << to_int(c);
                }
                ofs << "\"";
        }
        else
        {
                error("error type " + type + ". Must be bin or str.");
        }

        ofs << std::endl;

        return EXIT_SUCCESS;
}
