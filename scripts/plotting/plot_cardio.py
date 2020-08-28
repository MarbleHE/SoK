import math
from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import FuncFormatter
import itertools
import operator
from operator import add


def plot(labels: List[str], pandas_dataframes: List[pd.DataFrame], fig=None) -> plt.Figure:
    """

    :param labels:
    :param pandas_dataframes:
    :param fig:
    :return:
    """
    # Save current figure to restore later
    previous_figure = plt.gcf()

    # Set the current figure to fig
    # figsize = (int(len(labels) * 0.95), 6)
    figsize = (5, 4)
    config_dpi = 120
    if fig is None:
        fig = plt.figure(figsize=figsize, dpi=config_dpi)
    else:
        plt.rcParams["figure.figsize"] = figsize
        plt.rcParams["figure.dpi"] = config_dpi

    plt.figure(fig.number)

    # change size and DPI of resulting figure
    # change legend font size
    plt.rcParams["legend.fontsize"] = 8
    # NOTE: Enabling this requires latex to be installed on the Github actions runner
    # plt.rcParams["text.usetex"] = True
    plt.rcParams["font.family"] = 'serif'

    # Nice names for labels: maps folder name -> short name
    # and adds linebreaks where required
    # OPT_PARAMS_SIGN = '\\textsuperscript{$\dagger$}'
    # BASELINE_SIGN = '*'
    positions = {
        'Lobster-Baseline': (0, 0),
        'Lobster-Baseline-OPT': (1, 0),
        'MultiStart-OPT-PARAMS': (1, 1),
        'Lobster-OPT-PARAMS': (1, 2),
        'Cingulata-OPT': (2, 0),
        'SEAL-BFV': (3, 0),
        'TFHE': (4, 0),
        'SEAL-BFV-Batched': (5, 0)
    }

    # plt.title('Runtime for Cardio')
    plt.ylabel('Time (s)')

    bar_width = 0.03
    spacer = 0.02

    group_labels = [
        'Baseline', 'Depth Optim.', 'Cingulata', 'SEAL', 'TFHE', 'SEAL\n(batched)'
    ]

    # reserved_indices = {}
    # def get_x_position(group_no: int) -> int:
    #     next_idx = reserved_indices[group_no] + 1 if group_no in reserved_indices else 0
    #     reserved_indices[group_no] = next_idx
    #     return group_no + next_idx * bar_width

    def get_x_ticks_positions():
        group_widths = []
        x_pos_start = []
        x_pos_end = []
        for key, group in itertools.groupby(positions.values(), operator.itemgetter(0)):
            group_widths.append(len(list(group)) * bar_width)
        for w in group_widths:
            if not x_pos_start:
                x_pos_start.append(0 + spacer)
            else:
                x_pos_start.append(x_pos_end[-1] + spacer)
            x_pos_end.append(x_pos_start[-1] + w)
        result = list(map(add, x_pos_start, [w / 2 for w in group_widths]))
        return result, x_pos_start

    x_center, x_start = get_x_ticks_positions()

    def get_x_position(group_pos: tuple) -> int:
        return x_start[group_pos[0]] + (group_pos[1] * bar_width) + (bar_width / 2)

    plt.xticks(x_center, group_labels, fontsize=9)  # rotation='35',)
    # adds a thousand separator
    fig.axes[0].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    # add a grid
    ax = plt.gca()
    ax.grid(which='major', axis='y', linestyle=':')

    def ms_to_sec(num):
        return num / 1_000

    # Plot Bars
    max_y_value = 0
    for i in range(len(labels)):
        if not labels[i] in positions:
            continue
        else:
            x_pos = get_x_position(positions[labels[i]])
        df = pandas_dataframes[i]
        d1 = ms_to_sec(df['t_keygen'].mean())
        d1_err = 0 if math.isnan(df['t_keygen'].std()) else df['t_keygen'].std()
        p1 = plt.bar(x_pos, d1, bar_width * 0.9, color='red')
        d2 = ms_to_sec(df['t_input_encryption'].mean())
        d2_err = 0 if math.isnan(df['t_input_encryption'].std()) else df['t_input_encryption'].std()
        p2 = plt.bar(x_pos, d2, bar_width * 0.9, bottom=d1, color='blue')
        d3 = ms_to_sec(df['t_computation'].mean())
        d3_err = 0 if math.isnan(df['t_computation'].std()) else df['t_computation'].std()
        p3 = plt.bar(x_pos, d3, bar_width * 0.9, bottom=d1 + d2, color='green')
        d4 = ms_to_sec(df['t_decryption'].mean())
        d4_err = 0 if math.isnan(df['t_decryption'].std()) else df['t_decryption'].std()
        total_err = ms_to_sec(d1_err + d2_err + d3_err + d4_err)
        max_y_value = d1 + d2 + d3 + d4 if (d1 + d2 + d3 + d4) > max_y_value else max_y_value
        p4 = plt.bar(x_pos, d4, bar_width * 0.9, yerr=total_err, ecolor='black', capsize=5, bottom=d1 + d2 + d3,
                     color='cyan')
        print(labels[i].replace('\n', ' '), ": \n", d1, '\t', d2, '\t', d3, '\t', d4, '\t( total: ', d1 + d2 + d3 + d4,
              ')')

    max_y_rounded = (int(math.ceil(max_y_value / 10.0)) * 10) + 10
    plt.yticks(np.arange(0, max_y_rounded, step=10))

    # Add Legend
    plt.legend((p4[0], p3[0], p2[0], p1[0]), ('Decryption', 'Computation', 'Encryption', 'Key Generation'))

    # Restore current figure
    plt.figure(previous_figure.number)

    return fig


if __name__ == '__main__':
    print("Testing ploting with cardio example")
    data = [pd.read_csv('s3://sok-repository-eval-benchmarks/20200729_094952/Cingulata/cingulata_cardio.csv')]
    labels = ['Cingulata']
    plot(labels, data)
