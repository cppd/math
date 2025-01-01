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
import ast
import collections
import sys
import tempfile

import plotly.express as px
import plotly.graph_objects as go
from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox

FILE_PREFIX = "figure_"
FILE_SUFFIX = ".html"

POINT_SIZE_2D = 5
POINT_SIZE_3D = 1
SAMPLER_NAME_STRING = "Name:"
SAMPLER_GRID_STRING = "Grid:"
SAMPLER_LINE_COLOR = "#000000"
SAMPLER_LINE_WIDTH = 0.2

SamplerInfo = collections.namedtuple("SamplerInfo", ["name", "size"])


def error(message):
    raise Exception(message)


def sampler_title(title, sampler_info):
    assert sampler_info is not None
    return "{0}<br>{1}".format(title, sampler_info.name)


def sampler_lines(size):
    figures = []
    for i in range(0, size + 1):
        k = i / size
        figures.append(px.line(x=[k, k], y=[0, 1]))
        figures.append(px.line(x=[0, 1], y=[k, k]))
    for f in figures:
        f.update_traces(line_color=SAMPLER_LINE_COLOR, line_width=SAMPLER_LINE_WIDTH)
    return figures


def create_points_2d_sampler(points, title, sampler_info):
    assert len(points[0]) == 2
    assert sampler_info is not None

    x = [x for x, y in points]
    y = [y for x, y in points]

    figures = []

    f = px.scatter(x=x, y=y)
    f.update_traces(marker_size=POINT_SIZE_2D)
    figures.append(f)

    figures += sampler_lines(sampler_info.size)

    figure = go.Figure(data=sum((fig.data for fig in figures), ()))
    figure.update_xaxes(showgrid=False, visible=False)
    figure.update_yaxes(showgrid=False, visible=False)
    figure.update_layout(title=sampler_title(title, sampler_info))
    return figure


def create_points_2d(points, title):
    assert len(points[0]) == 2

    x = [x for x, y in points]
    y = [y for x, y in points]
    f = px.scatter(x=x, y=y)
    f.update_traces(marker_size=POINT_SIZE_2D)
    f.update_layout(title=title)
    return f


def create_points_3d(points, title, sampler_info):
    assert len(points[0]) == 3

    x = [x for x, y, z in points]
    y = [y for x, y, z in points]
    z = [z for x, y, z in points]
    f = px.scatter_3d(x=x, y=y, z=z)
    f.update_traces(marker_size=POINT_SIZE_3D)
    if sampler_info is None:
        f.update_layout(title=title)
    else:
        f.update_layout(title=sampler_title(title, sampler_info))
    return f


def show_points(sampler_info, points, title):
    if len(points) == 0:
        error("No points to show")

    if len(points[0]) not in (2, 3):
        error("{0}D is not supported".format(len(points[0])))

    if len(points[0]) == 2:
        if sampler_info is None:
            figure = create_points_2d(points, title)
        else:
            figure = create_points_2d_sampler(points, title, sampler_info)
    elif len(points[0]) == 3:
        figure = create_points_3d(points, title, sampler_info)
    else:
        assert False

    with tempfile.NamedTemporaryFile(delete=False, prefix=FILE_PREFIX, suffix=FILE_SUFFIX) as f:
        figure.write_html(f.name, auto_open=True)


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


def parse_sampler_name(text):
    word_begin = text.find(SAMPLER_NAME_STRING)
    if word_begin < 0:
        return None
    text = text[word_begin + len(SAMPLER_NAME_STRING) :].strip()
    if not text:
        return None
    return text


def parse_sampler_grid(text):
    word_begin = text.find(SAMPLER_GRID_STRING)
    if word_begin < 0:
        return None
    text = text[word_begin + len(SAMPLER_GRID_STRING) :].strip()
    words = text.split()
    if len(words) != 1:
        return None
    try:
        value = int(words[0])
    except ValueError:
        return None
    if not (value > 0):
        return None
    return value


def read_sampler_info(file_name):
    with open(file_name, encoding="utf-8") as file:
        line = file.readline()
        if not line:
            return None
        name = parse_sampler_name(line)
        if name is None:
            return None
        line = file.readline()
        if not line:
            return None
        grid = parse_sampler_grid(line)
        if not grid:
            return None
        line_count = 2
        return (line_count, name, grid)


def read_file(file_name):
    sampler_info = read_sampler_info(file_name)

    point_list = []
    dimension = None

    with open(file_name, encoding="utf-8") as file:
        if sampler_info is not None:
            for i in range(0, sampler_info[0]):
                file.readline()

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

    if sampler_info is None:
        return (None, point_list)
    return (SamplerInfo(*sampler_info[1:]), point_list)


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
        show_points(*read_file(name), name)
    except Exception as e:
        QMessageBox.critical(None, "Error", "{0}".format(e))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", nargs="?", type=str)
    parsed_args, unparsed_args = parser.parse_known_args()
    if parsed_args.file:
        show_points(*read_file(parsed_args.file), parsed_args.file)
    else:
        use_dialog(sys.argv[:1] + unparsed_args)
