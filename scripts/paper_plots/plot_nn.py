import math
from typing import List
import matplotlib.pyplot as plt
import pandas as pd
from brokenaxes import brokenaxes
from matplotlib.ticker import FuncFormatter

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
    fig_width = 252 * inches_per_pt  # width in inches
    # fig_height = (fig_width * golden_mean)  # height in inches
    fig_height = 2.5
    figsize = [fig_width * 0.67, fig_height / 1.22]

    # figsize = (5.5, 4)
    config_dpi = 100
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

    # Add fake EVA data if not present in data
    if 'EVA-MLP' not in labels:
        labels.append('EVA-MLP')
        pandas_dataframes.append(pd.DataFrame.from_dict({
            't_keygen': [0],
            't_input_encryption': [0],
            't_computation': [10000],
            't_decryption': [0]}
        ))
    if 'EVA-CHET' not in labels:
        labels.append('EVA-CHET')
        pandas_dataframes.append(pd.DataFrame.from_dict({
            't_keygen': [0],
            't_input_encryption': [0],
            't_computation': [10000],
            't_decryption': [0]}
        ))

    positions = {
        'SEAL-CKKS-Batched': (0, 0),

        'SEALion': (1, 0),

        'nGraph-HE-MLP': (2, 0),
        'nGraph-HE-Cryptonets': (2, 1),
        'nGraph-HE-LeNet5': (2, 2),

        'EVA-MLP': (3, 0),
        'EVA-CHET': (3, 1)
    }

    bar_width = 0.002
    spacer = 0.004
    inner_spacer = 0.0005

    group_labels = (
        'Manual\n{\\fontsize{7pt}{3em}\\selectfont{}(MLP)}',
        'SEALion\n{\\fontsize{7pt}{3em}\\selectfont{}(MLP)}',
        'nGraph-HE\n{\\fontsize{7pt}{3em}\\selectfont{}(MLP, CryptoNets, LeNet-5)}',
        'EVA\n{\\fontsize{7pt}{3em}\\selectfont{}(MLP, LeNet-5)}'
    )

    # Setup brokenaxes
    # TODO: Make break depend on value of nGraph-HE LeNet-5 runtime?
    # hspace controls how much space is in between the broken axes left=0.15
    # bax = brokenaxes(ylims=((0, 10), (120, 130)), hspace=.3, despine=False, left = 0.25, bottom = 0.25)
    bax = brokenaxes(ylims=((0, 10), (120, 140)), hspace=.4, despine=False, left=0.115, bottom=0.115)
    # bax = brokenaxes(ylims=((0, 10), (120, 130)), despine=False)

    # Setup Grids (0 is top, 1 is bottom part)
    bax.axs[0].grid(which='major', axis='y', linestyle=':')
    bax.axs[1].grid(which='major', axis='y', linestyle=':')

    def ms_to_sec(num):
        return num / 1_000

    colors = ['#15607a', '#ffbd70', '#e7e7e7', '#ff483a']

    x_center, x_start = get_x_ticks_positions(positions, bar_width, inner_spacer, spacer)
    # plt.xticks(x_center, group_labels, fontsize=9)

    # Plot Bars
    width = 0.002
    for i, label in enumerate(labels):
        df = pandas_dataframes[i]
        if len(df) == 0 or not labels[i] in positions:
            continue

        x_pos = get_x_position(positions[labels[i]], x_start, bar_width, inner_spacer)

        d1 = ms_to_sec(df['t_keygen'].mean())
        d1_err = 0 if math.isnan(df['t_keygen'].std()) else df['t_keygen'].std()
        p1 = bax.bar(x_pos, d1, width, color=colors[0])

        d2 = ms_to_sec(df['t_input_encryption'].mean())
        d2_err = 0 if math.isnan(df['t_input_encryption'].std()) else df['t_input_encryption'].std()
        p2 = bax.bar(x_pos, d2, width, bottom=d1, color=colors[1])

        d3 = ms_to_sec(df['t_computation'].mean())
        d3_err = 0 if math.isnan(df['t_computation'].std()) else df['t_computation'].std()
        p3 = bax.bar(x_pos, d3, width, bottom=d1 + d2, color=colors[2])

        d4 = ms_to_sec(df['t_decryption'].mean())
        d4_err = 0 if math.isnan(df['t_decryption'].std()) else df['t_decryption'].std()
        total_err = ms_to_sec(d1_err + d2_err + d3_err + d4_err)
        p4 = bax.bar(x_pos, d4, width, yerr=total_err, ecolor='black', capsize=3, bottom=d1 + d2 + d3,
                     color=colors[3])
        print(labels[i].replace('\n', ' '), ": \n", d1, '\t', d2, '\t', d3, '\t', d4, '\t( total: ',
              d1 + d2 + d3 + d4,
              ')')

    # Setup axes
    bax.axs[1].tick_params(axis='x', which='major', labelsize=9)
    bax.axs[0].tick_params(axis='y', which='major', labelsize=8)
    bax.axs[1].tick_params(axis='y', which='major', labelsize=8)
    bax.axs[0].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    bax.axs[1].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    # bax.axs[0].set_xticks(np.arange(len(group_labels)))
    bax.axs[0].set_xticks(x_center)
    # bax.axs[1].set_xticks(np.arange(len(group_labels)))
    bax.axs[1].set_xticks(x_center)
    bax.axs[1].set_xticklabels(group_labels)
    bax.set_ylabel('Time [s]', labelpad=26)

    # plt.title('Runtime for Neural Network Benchmark', fontsize=10)

    # Add Legend
    plt.legend((p4[0], p3[0], p2[0], p1[0]),
               ('Decryption', 'Computation', 'Encryption', 'Key Generation'),
               loc='upper left')

    # Restore current figure
    plt.figure(previous_figure.number)

    return fig


if __name__ == '__main__':
    print("Testing ploting with nn example")
    data = [pd.read_csv(
        's3://sok-repository-eval-benchmarks/20200830_125813__231137930/nGraph-HE-LeNet5/ngraph-he-lenet5-learned_nn.csv')]
    labels = ['nGraph-HE-LeNet5']
    fig = plot(labels, data)
    fig.savefig("nn_plot_test.pdf")
    fig.show()  # has some alignment issues, but pdf is fine
