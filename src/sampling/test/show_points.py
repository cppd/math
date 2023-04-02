# Copyright (C) 2017-2023 Topological Manifold
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
import ast
import sys
import tempfile

import plotly.express as px
from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox


def error(message):
    raise Exception(message)


def show_points(points, title):
    if len(points) == 0:
        error("No points to show")

    if len(points[0]) not in (2, 3):
        error("Point dimension {0} is not supported".format(len(points[0])))

    if len(points[0]) == 2:
        x = [x for x, y in points]
        y = [y for x, y in points]
        fig = px.scatter(x=x, y=y, title=title)
    elif len(points[0]) == 3:
        x = [x for x, y, z in points]
        y = [y for x, y, z in points]
        z = [z for x, y, z in points]
        fig = px.scatter_3d(x=x, y=y, z=z, title=title)
        fig.update_traces(marker_size=1)
    else:
        assert False

    file = tempfile.NamedTemporaryFile(delete=False)
    fig.write_html(file.name, auto_open=True)


# Convert string "*(x, y, ...)*" to tuple(x, y, ...)
def parse_sampler_point(text):
    point_begin = text.find("(")
    if point_begin < 0:
        error("Malformed point input:\n{0}".format(text))
    point_end = text.find(")", point_begin)
    if point_end < 0:
        error("Malformed point input:\n{0}".format(text))
    text = text[point_begin : (point_end + 1)]

    try:
        point = ast.literal_eval(text)
    except ValueError:
        error("Malformed point input:\n{0}".format(text))

    if not isinstance(point, tuple):
        error("Not tuple input:\n{0}".format(text))

    for coordinate in point:
        if not isinstance(coordinate, (int, float)):
            error("Not point input:\n{0}".format(text))

    return point


def read_file(file_name):
    point_list = []

    dimension = None

    with open(file_name, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            point = parse_sampler_point(line)

            if dimension is not None:
                if len(point) != dimension:
                    error("Inconsistent point dimensions {0} and {1}".format(dimension, len(point)))
            else:
                dimension = len(point)

            point_list.append(point)

    if not point_list:
        error("No points")

    if dimension is None:
        error("No dimension")

    return point_list


def use_dialog(args):
    app = QApplication(args)
    try:
        dialog = QFileDialog(None, "Open File")
        dialog.setNameFilter("*.txt")
        dialog.setFileMode(QFileDialog.FileMode.ExistingFile)
        dialog.setOption(QFileDialog.Option.DontUseNativeDialog, on=True)
        dialog.setOption(QFileDialog.Option.ReadOnly, on=True)
        if not dialog.exec():
            return
        name = dialog.selectedFiles()[0]
        show_points(read_file(name), name)
    except Exception as e:
        QMessageBox.critical(None, "Error", "{0}".format(e))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", nargs="?", type=str)
    parsed_args, unparsed_args = parser.parse_known_args()
    if parsed_args.file:
        show_points(read_file(parsed_args.file), parsed_args.file)
    else:
        use_dialog(sys.argv[:1] + unparsed_args)
