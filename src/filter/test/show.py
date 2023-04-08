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
import collections
import os
import sys
import tempfile

import plotly.graph_objects as go
from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox

FILE_PREFIX = "figure_"
FILE_SUFFIX = ".html"

MEASUREMENT_POINT_SIZE = 5
TRACK_COLOR = "#0000ff"
TRACK_LINE_WIDTH = 1

MEASUREMENT_COLOR = "#000000"
MEASUREMENT_LINE_WIDTH = 0.25
MEASUREMENT_MARKER_SIZE = 4


Point = collections.namedtuple("Point", ["x", "z"])


def error(message):
    raise Exception(message)


def create_points(points, title):
    assert len(points[0]) == 2
    assert len(points) > 0

    i = list(range(0, len(points)))
    x = [p.x for p in points]
    z = [p.z for p in points]

    figure = go.Figure()

    figure.add_trace(
        go.Scatter(
            x=i,
            y=x,
            name="Track",
            mode="lines",
            line=dict(color=TRACK_COLOR, width=TRACK_LINE_WIDTH, dash="dot"),
        )
    )

    figure.add_trace(
        go.Scatter(
            x=i,
            y=z,
            name="Measurements",
            mode="lines+markers",
            marker_size=MEASUREMENT_MARKER_SIZE,
            line=dict(color=MEASUREMENT_COLOR, width=MEASUREMENT_LINE_WIDTH),
        )
    )

    figure.update_xaxes(showgrid=True, visible=True)
    figure.update_yaxes(showgrid=True, visible=True)
    figure.update_layout(title=title, xaxis_title="Time", yaxis_title="Position")
    return figure


def show_points(points, file_name):
    if len(points) == 0:
        error("No points to show")

    if not (len(points[0]) == 2):
        error("Not supported data size {0}".format(len(points[0])))

    figure = create_points(points, title=os.path.basename(file_name))

    file = tempfile.NamedTemporaryFile(delete=False, prefix=FILE_PREFIX, suffix=FILE_SUFFIX)
    figure.write_html(file.name, auto_open=True)


def parse_point(text):
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

    return Point(*point)


def read_file(file_name):
    point_list = []
    dimension = None

    with open(file_name, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            point = parse_point(line)

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
