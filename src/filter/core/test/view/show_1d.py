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
import os
import sys
import tempfile

import plotly.graph_objects as go
from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox

FILE_PREFIX = "figure_"
FILE_SUFFIX = ".html"


def error(message):
    raise Exception(message)


def add_stddev_line(figure, d, x, y):
    figure.add_trace(
        go.Scatter(
            x=x,
            y=y,
            name=d["name"],
            legendgroup=d["name"],
            showlegend=False,
            mode=d["mode"],
            marker_size=d["marker_size"],
            line={"color": d["line_color"], "width": d["line_width"], "dash": d["line_dash"]},
        )
    )


def add_stddev(figure, d, values):
    assert len(values) > 0
    assert len(values[0]) == 3

    x = [p[0] for p in values]
    lower = [p[1] - p[2] for p in values]
    upper = [p[1] + p[2] for p in values]

    add_stddev_line(figure, d, x, lower)
    add_stddev_line(figure, d, x, upper)

    figure.add_trace(
        go.Scatter(
            x=x + x[::-1],
            y=upper + lower[::-1],
            name=d["name"],
            legendgroup=d["name"],
            showlegend=True,
            fill="toself",
            fillcolor=d["fill_color"],
            line_color="rgba(0,0,0,0)",
        )
    )


def add_line(figure, d, values):
    assert len(values) > 0
    assert len(values[0]) == 2

    figure.add_trace(
        go.Scatter(
            x=[p[0] for p in values],
            y=[p[1] for p in values],
            name=d["name"],
            mode=d["mode"],
            marker_size=d["marker_size"],
            line={"color": d["line_color"], "width": d["line_width"], "dash": d["line_dash"]},
        )
    )


def create_figure(info, title):
    assert info and isinstance(info, list)

    figure = go.Figure()

    for d, values in info:
        assert len(values) > 0
        assert len(values[0]) == 2 or len(values[0]) == 3
        if len(values[0]) == 2:
            add_line(figure, d, values)
        else:
            add_stddev(figure, d, values)

    figure.update_xaxes(showgrid=True, visible=True)
    figure.update_yaxes(showgrid=True, visible=True)

    title = None
    figure.update_layout(title=title, xaxis_title="Time", yaxis_title="Position")
    return figure


def show_data(info, file_name):
    if not info:
        error("No info to show")

    figure = create_figure(info, title=os.path.basename(file_name))

    file = tempfile.NamedTemporaryFile(delete=False, prefix=FILE_PREFIX, suffix=FILE_SUFFIX)
    figure.write_html(file.name, auto_open=True)


def parse_data(text):
    try:
        data = ast.literal_eval(text)
    except ValueError:
        error("Malformed input:\n{0}".format(text))

    if isinstance(data, dict):
        return data

    if not isinstance(data, tuple):
        error("Not tuple input:\n{0}".format(text))

    for d in data:
        if not (d is None or isinstance(d, float)):
            error("Input type error:\n{0}".format(text))

    return data


def read_file(file_name):
    res = []
    dimension = None
    values = None

    with open(file_name, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            data = parse_data(line)

            if isinstance(data, dict):
                res.append((data, []))
                values = res[-1][1]
                dimension = None
                continue

            if dimension is not None:
                if len(data) != dimension:
                    error("Inconsistent dimensions {0} and {1}".format(dimension, len(data)))
            else:
                dimension = len(data)

            if values is None:
                error("No data name")

            values.append(data)

    if not res:
        error("No data")

    if dimension is None:
        error("No dimension")

    return res


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
