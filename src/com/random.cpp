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

#include "random.h"

#if defined(__linux__)

#include "error.h"

#include <fstream>

// Взять случайные данные из файла ОС вместо использования std::random_device
void read_system_random(void* p, unsigned count)
{
        constexpr const char* DEV_RANDOM = "/dev/urandom";

        std::ifstream rnd(DEV_RANDOM);
        if (!rnd)
        {
                error(std::string("error open file ") + DEV_RANDOM);
        }

        rnd.read(reinterpret_cast<char*>(p), count);
        if (!rnd)
        {
                error(std::string("error read from file ") + DEV_RANDOM);
        }
}

#elif defined(_WIN32)

#include "error.h"

// Вначале windows.h
#include <windows.h>
// Потом wincrypt.h
#include <wincrypt.h>

class Provider
{
        HCRYPTPROV m_hProvider;

public:
        Provider()
        {
                if (!::CryptAcquireContext(&m_hProvider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                {
                        error("error CryptAcquireContext");
                }
        }
        ~Provider()
        {
                if (!::CryptReleaseContext(m_hProvider, 0))
                {
                        error("error CryptReleaseContext");
                }
        }

        operator HCRYPTPROV() const
        {
                return m_hProvider;
        }
};

void read_system_random(void* p, unsigned count)
{
        Provider provider;
        if (!::CryptGenRandom(provider, count, reinterpret_cast<BYTE*>(p)))
        {
                error("error CryptGenRandom");
        }
}

#else

#error This operating system is not supported

#endif
