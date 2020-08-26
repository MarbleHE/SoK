from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.ticker import FuncFormatter


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
    if fig is None:
        fig = plt.figure()
    plt.figure(fig.number)

    # change size and DPI of resulting figure
    plt.rcParams["figure.figsize"] = (8, 6)
    plt.rcParams["figure.dpi"] = 90
    # change legend font size
    plt.rcParams["legend.fontsize"] = 8

    # Nice names for labels: maps folder name -> short name
    # and adds linebreaks where required
    subs = {'Cingulata-UNOPT': 'Cingulata\n(unopt.)',
            'SEAL-BFV-Batched': 'SEAL-BFV\n(batched)',
            'SEAL-CKKS-Batched': 'SEAL-CKKS\n(batched)'}
    labels = [subs.get(item, item) for item in labels]

    # Setup Axis, Title, etc
    N = len(labels)
    # plt.title('Runtime for NN Inference')
    plt.ylabel('Time (s)')
    ind = np.arange(N)  # the x locations for the groups
    plt.xticks(ind, labels, fontsize=9)
    plt.yticks(np.arange(0, 100, step=1))
    # adds a thousand separator
    fig.axes[0].get_yaxis().set_major_formatter(FuncFormatter(lambda x, p: format(int(x), ',')))
    width = 0.35  # the width of the bars: can also be len(x) sequence
    # add a grid
    ax = plt.gca()
    ax.grid(which='major', axis='y', linestyle=':')

    def ms_to_sec(num):
        return num/1_000

    # Plot Bars
    for i in range(N):
        df = pandas_dataframes[i]
        d1 = ms_to_sec(df['t_keygen'].mean())
        d1_err = df['t_keygen'].std()
        p1 = plt.bar(ind[i], d1, width, color='red')
        d2 = ms_to_sec(df['t_input_encryption'][i].mean())
        d2_err = df['t_input_encryption'][i].std()
        p2 = plt.bar(ind[i], d2, width, bottom=d1, color='blue')
        d3 = ms_to_sec(df['t_computation'][i].mean())
        d3_err = df['t_computation'][i].std()
        p3 = plt.bar(ind[i], d3, width, bottom=d1 + d2, color='green')
        d4 = ms_to_sec(df['t_decryption'][i].mean())
        d4_err = df['t_decryption'][i].std()
        total_err = ms_to_sec(d1_err + d2_err + d3_err + d4_err)
        # if total_err > 500:
        p4 = plt.bar(ind[i], d4, width, yerr=total_err, ecolor='black', capsize=5, bottom=d1 + d2 + d3, color='cyan')
        # else:
        #     p4 = plt.bar(ind[i], d4, width, bottom=d1 + d2 + d3, color='cyan')
        print(labels[i], ": ", d1+d2+d3+d4)

    # Add Legend
    plt.legend((p4[0], p3[0], p2[0], p1[0]), ('Decryption', 'Computation', 'Encryption', 'Key Generation'))

    # Restore current figure
    plt.figure(previous_figure.number)

    return fig


if __name__ == '__main__':
    print("Testing ploting with nn example")
    data = [pd.read_csv('s3://sok-repository-eval-benchmarks/20200825_211505/SEALion/sealion_nn.csv')]
    labels = ['SEALion']
    plot(labels, data).show()
