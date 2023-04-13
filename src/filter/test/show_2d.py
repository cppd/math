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

TRACK_COLOR = "#0000ff"
TRACK_LINE_WIDTH = 1

MEASUREMENT_COLOR = "#000000"
MEASUREMENT_LINE_WIDTH = 0.25
MEASUREMENT_MARKER_SIZE = 4


Data = collections.namedtuple("Data", ["x0", "x1", "z0", "z1"])


def error(message):
    raise Exception(message)


def create_figure(data, title):
    assert len(data) > 0

    figure = go.Figure()

    figure.add_trace(
        go.Scatter(
            x=[p.x0 for p in data],
            y=[p.x1 for p in data],
            name="Track",
            mode="lines",
            line=dict(color=TRACK_COLOR, width=TRACK_LINE_WIDTH, dash="dot"),
        )
    )

    figure.add_trace(
        go.Scatter(
            x=[p.z0 for p in data],
            y=[p.z1 for p in data],
            name="Measurements",
            mode="lines+markers",
            marker_size=MEASUREMENT_MARKER_SIZE,
            line=dict(color=MEASUREMENT_COLOR, width=MEASUREMENT_LINE_WIDTH),
        )
    )

    figure.update_xaxes(showgrid=True, visible=True)
    figure.update_yaxes(showgrid=True, visible=True)
    figure.update_layout(title=title)
    return figure


def show_data(data, file_name):
    if len(data) == 0:
        error("No data to show")

    figure = create_figure(data, title=os.path.basename(file_name))

    file = tempfile.NamedTemporaryFile(delete=False, prefix=FILE_PREFIX, suffix=FILE_SUFFIX)
    figure.write_html(file.name, auto_open=True)


def parse_data(text):
    begin = text.find("(")
    if begin < 0:
        error("Malformed input:\n{0}".format(text))
    end = text.find(")", begin)
    if end < 0:
        error("Malformed input:\n{0}".format(text))
    text = text[begin : (end + 1)]

    try:
        data = ast.literal_eval(text)
    except ValueError:
        error("Malformed input:\n{0}".format(text))

    if not isinstance(data, tuple):
        error("Not tuple input:\n{0}".format(text))

    for d in data:
        if not isinstance(d, (int, float)):
            error("Input type error:\n{0}".format(text))

    return Data(*data)


def read_file(file_name):
    data_list = []
    dimension = None

    with open(file_name, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            data = parse_data(line)

            if dimension is not None:
                if len(data) != dimension:
                    error("Inconsistent dimensions {0} and {1}".format(dimension, len(data)))
            else:
                dimension = len(data)

            data_list.append(data)

    if not data_list:
        error("No data")

    if dimension is None:
        error("No dimension")

    return data_list


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
        show_data(read_file(name), name)
    except Exception as e:
        QMessageBox.critical(None, "Error", "{0}".format(e))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", nargs="?", type=str)
    parsed_args, unparsed_args = parser.parse_known_args()
    if parsed_args.file:
        show_data(read_file(parsed_args.file), parsed_args.file)
    else:
        use_dialog(sys.argv[:1] + unparsed_args)
