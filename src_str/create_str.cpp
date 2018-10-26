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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

constexpr int LINE_LENGTH_STR = 24;
constexpr int LINE_LENGTH_BIN = 16;
constexpr int LINE_LENGTH_SPR = 8;

constexpr const char TYPE_STR[] = "str";
constexpr const char TYPE_BIN[] = "bin";
constexpr const char TYPE_SPR[] = "spr";

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
[[noreturn]] void error(const std::string& msg)
{
        std::cerr << msg << std::endl;
        std::exit(EXIT_FAILURE);
}

int to_int(char c)
{
        return static_cast<unsigned char>(c);
}

enum class ConversionType
{
        Str,
        Bin,
        Spr
};

ConversionType conversion_type_string_to_enum(const char* t)
{
        if (!std::strcmp(t, TYPE_STR))
        {
                return ConversionType::Str;
        }

        if (!std::strcmp(t, TYPE_BIN))
        {
                return ConversionType::Bin;
        }

        if (!std::strcmp(t, TYPE_SPR))
        {
                return ConversionType::Spr;
        }

        error("Error conversion type \"" + std::string(t) + "\"");
}

void str(std::ifstream& ifs, std::ofstream& ofs)
{
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
}

void bin(std::ifstream& ifs, std::ofstream& ofs)
{
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
}

void spr(std::ifstream& ifs, std::ofstream& ofs)
{
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
}
}

int main(int argc, char* argv[])
{
        if (argc != 4)
        {
                error("Usage: program " + std::string(TYPE_STR) + "|" + std::string(TYPE_BIN) + "|" + std::string(TYPE_SPR) +
                      " file_in file_out");
        }

        const ConversionType conversion_type = conversion_type_string_to_enum(argv[1]);
        const char* const input_name = argv[2];
        const char* const output_name = argv[3];

        std::ifstream ifs;

        switch (conversion_type)
        {
        case ConversionType::Str:
                ifs = std::ifstream(input_name);
                break;
        case ConversionType::Bin:
                ifs = std::ifstream(input_name, std::ios_base::binary);
                break;
        case ConversionType::Spr:
                ifs = std::ifstream(input_name, std::ios_base::binary);
                break;
        }

        std::ofstream ofs(output_name);

        if (!ifs)
        {
                error("Error opening input file \"" + std::string(input_name) + "\"");
        }

        if (!ofs)
        {
                error("Error opening output file \"" + std::string(output_name) + "\"");
        }

        ofs << std::hex << std::setfill('0');

        switch (conversion_type)
        {
        case ConversionType::Str:
                str(ifs, ofs);
                break;
        case ConversionType::Bin:
                bin(ifs, ofs);
                break;
        case ConversionType::Spr:
                spr(ifs, ofs);
                break;
        }

        ofs << std::endl;

        if (!ofs)
        {
                error("Error writing to output file \"" + std::string(output_name) + "\"");
        }

        return EXIT_SUCCESS;
}
