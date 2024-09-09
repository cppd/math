# Copyright (C) 2017-2024 Topological Manifold
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
from collections import namedtuple

import plotly.graph_objects as go
from PyQt6.QtWidgets import QApplication, QFileDialog, QMessageBox

FILE_PREFIX = "figure_"
FILE_SUFFIX = ".html"


Info = namedtuple("Info", "annotation data")


def error(message):
    raise Exception(message)


def show_data(info):

    data = info.data
    assert len(data) > 0

    figure = go.Figure()

    figure.add_trace(go.Scatter(x=[p[0] for p in data], y=[p[1] for p in data], name="Allan Deviation", mode="lines"))

    figure.update_xaxes(showgrid=True, visible=True, type="log")
    figure.update_yaxes(showgrid=True, visible=True, type="log")

    if info.annotation:
        figure.add_annotation(
            text=info.annotation,
            align="left",
            showarrow=False,
            xref="paper",
            yref="paper",
            x=1.02,
            y=0.01,
            xanchor="left",
            yanchor="bottom",
        )

    figure.update_layout(title=None, xaxis_title="\u03C4", yaxis_title="Deviation", showlegend=True)
    file = tempfile.NamedTemporaryFile(delete=False, prefix=FILE_PREFIX, suffix=FILE_SUFFIX)
    figure.write_html(file.name, auto_open=True)


def parse_data(text):
    try:
        data = ast.literal_eval(text)
    except ValueError:
        error("Malformed input:\n{0}".format(text))

    if isinstance(data, str):
        return data

    if not isinstance(data, tuple):
        error("Not tuple input:\n{0}".format(text))

    for d in data:
        if not (d is None or isinstance(d, float)):
            error("Input type error:\n{0}".format(text))

    return data


def read_file(file_name):
    res = []
    annotation = None

    with open(file_name, encoding="utf-8") as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            data = parse_data(line)

            if isinstance(data, str):
                if annotation is not None:
                    error("Too many annotations")
                annotation = data
                continue

            if len(data) != 2:
                error("Dimension ({0}) must be equal to 2".format(len(data)))

            res.append(data)

    if not res:
        error("No data")

    return Info(annotation, res)


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
        show_data(read_file(name))
    except Exception as e:
        QMessageBox.critical(None, "Error", "{0}".format(e))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", nargs="?", type=str)
    parsed_args, unparsed_args = parser.parse_known_args()
    if parsed_args.file:
        show_data(read_file(parsed_args.file))
    else:
        use_dialog(sys.argv[:1] + unparsed_args)
