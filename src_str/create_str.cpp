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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

constexpr int LINE_LENGTH_STR = 24;
constexpr int LINE_LENGTH_BIN = 16;
constexpr int LINE_LENGTH_SPR = 8;

constexpr const char* COMMAND_STR = "str";
constexpr const char* COMMAND_BIN = "bin";
constexpr const char* COMMAND_SPR = "spr";
constexpr const char* COMMAND_CAT = "cat";

// SPIR-V Specification
// 3.1 Magic Number
constexpr uint32_t SPR_MAGIC_NUMBER = 0x07230203;

constexpr uint32_t bswap32(uint32_t n)
{
        return __builtin_bswap32(n);
}

static_assert(bswap32(0x12345678) == 0x78563412);

namespace
{
[[noreturn]] void error(const std::string_view& msg) noexcept
{
        std::cerr << msg << std::endl;
        std::exit(EXIT_FAILURE);
}

std::string usage()
{
        std::string s;
        s += "Usage:\n";
        s += "program " + std::string(COMMAND_STR) + "|" + std::string(COMMAND_BIN) + "|" + std::string(COMMAND_SPR)
             + " file_in file_out\n";
        s += "program " + std::string(COMMAND_CAT) + " files_in file_out";
        return s;
}

int to_int(char c)
{
        return static_cast<unsigned char>(c);
}

class ifstream final : public std::ifstream
{
public:
        explicit ifstream(const char* name, std::ios_base::openmode mode = ios_base::in) : std::ifstream(name, mode)
        {
                if (fail())
                {
                        error("Error opening input file \"" + std::string(name) + "\"");
                }
        }
};

class ofstream final : public std::ofstream
{
        std::string m_name;

public:
        explicit ofstream(const char* name, std::ios_base::openmode mode = ios_base::out) : std::ofstream(name, mode)
        {
                if (fail())
                {
                        error("Error opening output file \"" + std::string(name) + "\"");
                }
                m_name = name;
        }

        ~ofstream() override
        {
                if (fail())
                {
                        error("Error writing to output file \"" + m_name + "\"");
                }
        }
};

void str(const char* input_name, const char* output_name)
{
        ifstream ifs(input_name);
        ofstream ofs(output_name);

        ofs << std::hex << std::setfill('0');

        long long i = 0;

        ofs << "\"";
        for (char c; ifs.get(c); ++i)
        {
                if (i != 0)
                {
                        if (i % LINE_LENGTH_STR == 0)
                        {
                                ofs << "\"\n\"";
                        }
                }
                ofs << "\\x" << std::setw(2) << to_int(c);
        }
        ofs << "\"";

        ofs << std::endl;
}

void bin(const char* input_name, const char* output_name)
{
        ifstream ifs(input_name, std::ios_base::binary);
        ofstream ofs(output_name);

        ofs << std::hex << std::setfill('0');

        long long i = 0;

        for (char c; ifs.get(c); ++i)
        {
                if (i != 0)
                {
                        ofs << ',';
                        if (i % LINE_LENGTH_BIN != 0)
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

        ofs << std::endl;
}

void spr(const char* input_name, const char* output_name)
{
        ifstream ifs(input_name, std::ios_base::binary);
        ofstream ofs(output_name);

        ofs << std::hex << std::setfill('0');

        long long i = 0;

        bool reverse_byte_order = false;

        for (uint32_t n; ifs.read(reinterpret_cast<char*>(&n), sizeof(n)); ++i)
        {
                if (i == 0)
                {
                        if (n != SPR_MAGIC_NUMBER)
                        {
                                if (bswap32(n) == SPR_MAGIC_NUMBER)
                                {
                                        reverse_byte_order = true;
                                }
                                else
                                {
                                        error("Error reading SPIR-V (no magic number)");
                                }
                        }
                }
                else
                {
                        ofs << ',';
                        if (i % LINE_LENGTH_SPR != 0)
                        {
                                ofs << ' ';
                        }
                        else
                        {
                                ofs << '\n';
                        }
                }

                ofs << "0x" << std::setw(8) << (!reverse_byte_order ? n : bswap32(n));
        }

        if (ifs.gcount() != 0)
        {
                error("Error reading SPIR-V (code size is not a multiple of 4)");
        }

        if (i == 0)
        {
                error("Error reading SPIR-V (empty file)");
        }

        ofs << std::endl;
}

void cat(const std::vector<const char*>& input_names, const char* output_name)
{
        std::string s;

        for (const char* name : input_names)
        {
                ifstream ifs(name, std::ios_base::binary);

                char c;
                while (ifs.get(c))
                {
                        s += c;
                }
        }

        ofstream ofs(output_name, std::ios_base::binary);
        ofs.write(s.c_str(), s.size());
}
}

int main(int argc, char* argv[])
{
        if (argc < 2)
        {
                error(usage());
        }

        const char* const command = argv[1];

        if (!std::strcmp(command, COMMAND_STR))
        {
                if (argc != 4)
                {
                        error(usage());
                }

                str(argv[2], argv[3]);
        }
        else if (!std::strcmp(command, COMMAND_BIN))
        {
                if (argc != 4)
                {
                        error(usage());
                }

                bin(argv[2], argv[3]);
        }
        else if (!std::strcmp(command, COMMAND_SPR))
        {
                if (argc != 4)
                {
                        error(usage());
                }

                spr(argv[2], argv[3]);
        }
        else if (!std::strcmp(command, COMMAND_CAT))
        {
                if (argc < 4)
                {
                        error(usage());
                }

                cat({argv + 2, argv + argc - 1}, argv[argc - 1]);
        }
        else
        {
                error(usage());
        }

        return EXIT_SUCCESS;
}
