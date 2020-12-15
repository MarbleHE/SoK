import math
from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import FuncFormatter
import itertools
import operator
from operator import add


def human_format(num):
    num = float('{:.3g}'.format(num))
    magnitude = 0
    while abs(num) >= 1000:
        magnitude += 1
        num /= 1000.0
    return '{}{}'.format('{:f}'.format(num).rstrip('0').rstrip('.'), ['', 'K', 'M', 'B', 'T'][magnitude])


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
    golden_mean = ((np.math.sqrt(5) - 1.0) / 2.0) * .8  # Aesthetic ratio
    # fig_width = 252 * inches_per_pt  # width in inches
    fig_width = 384 * inches_per_pt  # width in inches
    # fig_height = (fig_width * golden_mean)  # height in inches
    # fig_height = 2.5
    fig_height = 4.5
    fig_size = [fig_width * 0.67, fig_height / 1.22]

    config_dpi = 160
    if fig is None:
        fig = plt.figure(figsize=fig_size, dpi=config_dpi)
    else:
        plt.rcParams["figure.figsize"] = fig_size
        plt.rcParams["figure.dpi"] = config_dpi

    plt.figure(fig.number)

    # change size and DPI of resulting figure
    # change legend font size
    plt.rcParams["legend.fontsize"] = 8
    # NOTE: Enabling this requires latex to be installed on the Github actions runner
    # plt.rcParams["text.usetex"] = True
    plt.rcParams["font.family"] = 'serif'

    positions = {
        'Mul. (C,C)': (0, 0),
        'Mul. (C,P)': (1, 0),
        'Add. (C,C)': (2, 0),
        'Add. (C,P)': (3, 0),
        'Enc. (SK)': (4, 0),
        'Enc. (PK)': (5, 0),
        'Dec.': (6, 0),
        'Rot.': (7, 0)
    }

    # plt.title('Runtime for Chi-Squared Test Benchmark', fontsize=10)
    plt.ylabel('Time [s]', labelpad=0)

    # adds a thousand separator
    # fig.axes[0].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    # add a grid
    ax = plt.gca()
    ax.grid(which='major', axis='y', linestyle=':')

    def us_to_msec(num):
        return num / 1_000

    tools = {
        "PALISADE-Microbenchmark": "PALISADE",
        "SEAL-BFV-Microbenchmark": "SEAL-BFV",
    }
    colors = ['0.1', '0.35', '0.5', '0.85']
    hatches = ['', '.', '///', '']

    data = pd.DataFrame(index=pandas_dataframes[0].columns)  #
    for v, l in zip(pandas_dataframes, labels):
        data[tools[l]] = v.values.tolist()[0]

    x_labels = {
        't_mul_ct_ct': 'mul\n(ct,ct)',
        't_mul_ct_ct_inplace': 'mul ip\n(ct,ct)',
        't_mul_ct_pt': 'mul\n(ct,pt)',
        't_mul_ct_pt_inplace': 'mul ip\n(ct,pt)',
        't_add_ct_ct': 'add\n(ct,ct)',
        't_add_ct_ct_inplace': 'add ip\n(ct,ct)',
        't_add_ct_pt': 'add\n(ct,pt)',
        't_add_ct_pt_inplace': 'add ip\n(ct,pt)',
        't_enc_sk': 'enc\n(sk)',
        't_enc_pk': 'enc\n(pk)',
        't_dec': 'dec',
        't_rot': 'rot',
    }
    old_keys = list(data.index)
    new_keys = []
    for k in old_keys:
        new_keys.append(x_labels[k])
    data.index = new_keys

    ax = data.plot.bar(color=colors, width=0.4, logy=True, rot=0, ax=ax)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda x, p: human_format(x)))

    plt.tight_layout()

    plt.show()

    # Restore current figure
    plt.figure(previous_figure.number)

    return fig
