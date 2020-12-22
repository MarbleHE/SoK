import math
from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import FuncFormatter
import itertools
import operator
from operator import add
import matplotlib.colors as mcolors

from plot_utils import get_x_ticks_positions, get_x_position


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
    inches_per_pt = 1.0 / 72.27 * 2  # Convert pt to inches
    fig_width = 275 * inches_per_pt  # width in inches
    # fig_height = (fig_width * golden_mean)  # height in inches
    fig_height = 2.5
    figsize = [fig_width * 0.67, fig_height / 1.22]

    config_dpi = 100
    # figsize = (252/config_dpi, 100/config_dpi)
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
    plt.rcParams["text.usetex"] = True
    plt.rcParams["font.family"] = 'serif'

    # Nice names for labels: maps folder name -> short name
    # and adds linebreaks where required
    # OPT_PARAMS_SIGN = '\\textsuperscript{$\dagger$}'
    # BASELINE_SIGN = '*'
    positions = {
        'Lobster-Baseline': (0, 0),
        'Lobster-Baseline-OPT': (0, 1),
        'MultiStart-OPT-PARAMS': (0, 2),
        'Lobster-OPT-PARAMS': (0, 3),

        'Cingulata-OPT': (1, 0),

        'SEAL-BFV-Manualparams': (2, 0),
        'SEAL-BFV-Naive-Sealparams': (2, 1),
        'E3-SEAL': (2, 2),

        'TFHE': (3, 0),
        'E3-TFHE': (3, 1),

        'SEAL-BFV-Batched-Manualparams': (4, 0),
        'E3-SEAL-Batched': (4, 1)
    }

    # plt.title('Runtime for Cardio Benchmark', fontsize=10)

    bar_width = 0.002
    spacer = 0.004
    inner_spacer = 0.0005

    group_labels = [
        'Depth Optimized\n{\\fontsize{6pt}{3em}\\selectfont{(A/B/C/D})}',
        'Cingu.',
        'SEAL\n{\\fontsize{6pt}{3em}\\selectfont{}(Opt./Naive/E\\textsuperscript{3})}',
        'TFHE\n{\\fontsize{6pt}{3em}\\selectfont{}(Opt./E\\textsuperscript{3})}',
        'SEAL Bat.\n{\\fontsize{6pt}{3em}\\selectfont{}(Opt./E\\textsuperscript{3})}'
    ]

    x_center, x_start = get_x_ticks_positions(positions, bar_width, inner_spacer, spacer)

    plt.xticks(x_center, group_labels, fontsize=9)  # rotation='35',)
    # adds a thousand separator
    fig.axes[0].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    # add a grid
    ax = plt.gca()
    ax.grid(which='major', axis='y', linestyle=':')

    def ms_to_sec(num):
        return num / 1_000

    # colors = ['0.1', '0.35', '0.5', '0.85']
    # hatches = ['', '.', '///', '']

    colors = ['#15607a', '#ffbd70', '#e7e7e7', '#ff483a']

    # Plot Bars
    max_y_value = 0
    for i in range(len(labels)):
        if not labels[i] in positions:
            continue
        else:
            x_pos = get_x_position(positions[labels[i]], x_start, bar_width, inner_spacer)
        df = pandas_dataframes[i]
        d1 = ms_to_sec(df['t_keygen'].mean())
        d1_err = 0 if math.isnan(df['t_keygen'].std()) else df['t_keygen'].std()
        p1 = plt.bar(x_pos, d1, bar_width * 0.9, color=colors[0])
        d2 = ms_to_sec(df['t_input_encryption'].mean())
        d2_err = 0 if math.isnan(df['t_input_encryption'].std()) else df['t_input_encryption'].std()
        p2 = plt.bar(x_pos, d2, bar_width * 0.9, bottom=d1, color=colors[1])
        d3 = ms_to_sec(df['t_computation'].mean())
        d3_err = 0 if math.isnan(df['t_computation'].std()) else df['t_computation'].std()
        p3 = plt.bar(x_pos, d3, bar_width * 0.9, bottom=d1 + d2, color=colors[2])
        d4 = ms_to_sec(df['t_decryption'].mean())
        d4_err = 0 if math.isnan(df['t_decryption'].std()) else df['t_decryption'].std()
        total_err = ms_to_sec(d1_err + d2_err + d3_err + d4_err)
        max_y_value = d1 + d2 + d3 + d4 if (d1 + d2 + d3 + d4) > max_y_value else max_y_value
        p4 = plt.bar(x_pos, d4, bar_width * 0.9, yerr=total_err, ecolor='black', capsize=3, bottom=d1 + d2 + d3,
                     color=colors[3])
        print(labels[i].replace('\n', ' '), ": \n", d1, '\t', d2, '\t', d3, '\t', d4, '\t( total: ', d1 + d2 + d3 + d4,
              ')')

    plt.ylabel('Time [s]', labelpad=0)
    max_y_rounded = (int(math.ceil(max_y_value / 10.0)) * 10) + 10
    plt.yticks(fontsize=8)

    # Add Legend
    plt.legend((p4[0], p3[0], p2[0], p1[0]),
               ('Dec.', 'Comp.', 'Enc.', 'Key Gen.'),
               ncol=2, loc='upper left', fontsize=7)

    # Restore current figure
    plt.figure(previous_figure.number)

    return fig


if __name__ == '__main__':
    print("Testing ploting with cardio example")
    data = [pd.read_csv('s3://sok-repository-eval-benchmarks/20200729_094952/Cingulata/cingulata_cardio.csv')]
    labels = ['Cingulata']
    plot(labels, data)
