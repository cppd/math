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

#if 1

#include "init/init.h"
#include "qt/qt_main.h"

int main(int argc, char* argv[])
{
        init();

        return qt_main(argc, argv);
}

#else

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"

int main()
{
}

#endif
