# Copyright (C) 2017-2025 Topological Manifold
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import glob
import multiprocessing
import os
import subprocess

CLANG_FORMAT_EXTENSIONS = {".h", ".hpp", ".c", ".cpp", ".comp", ".vert", ".frag", ".geom", ".tesc", ".tese", ".glsl"}
CLANG_FORMAT_OPTIONS = ["-i", "-style=file"]

PYTHON_EXTENSIONS = {".py"}
ISORT = ["isort", "-q"]
BLACK = ["black", "-q"]


def process_files(files, clang_format, process_number, process_count):
    for i in range(process_number, len(files), process_count):
        file = files[i]
        if not os.path.isfile(file):
            continue
        _, ext = os.path.splitext(file)
        if ext in CLANG_FORMAT_EXTENSIONS:
            subprocess.call([clang_format] + CLANG_FORMAT_OPTIONS + [file])
        elif ext in PYTHON_EXTENSIONS:
            subprocess.call(ISORT + [file])
            subprocess.call(BLACK + [file])


def run(clang_format):
    process_count = multiprocessing.cpu_count()
    files = glob.glob("**", recursive=True)

    processes = []

    for process_number in range(0, process_count):
        process_args = (files, clang_format, process_number, process_count)
        process = multiprocessing.Process(target=process_files, args=process_args)
        process.start()
        processes.append(process)

    for process in processes:
        process.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("clang-format", type=str)
    args = parser.parse_args()
    run(getattr(args, "clang-format"))
