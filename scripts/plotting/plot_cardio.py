from typing import List
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np



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

    # Setup Axis, Title, etc
    N = len(labels)
    plt.title('Runtime for Cardio')
    plt.ylabel('Time (ms)')
    ind = np.arange(N)  # the x locations for the groups
    plt.xticks(ind, labels)
    width = 0.35  # the width of the bars: can also be len(x) sequence

    # Plot Bars
    for i in range(N):
        df = pandas_dataframes[i]
        d1 = df['t_keygen'].mean()
        p1 = plt.bar(ind[i], d1, width, color='red')
        d2 = df['t_input_encryption'][i].mean()
        p2 = plt.bar(ind[i], d2 , width, bottom=d1, color='blue')
        d3 = df['t_computation'][i].mean()
        p3 = plt.bar(ind[i], d3, width, bottom=d1+d2, color='green')
        d4 = df['t_decryption'][i].mean()
        p4 = plt.bar(ind[i], d4, width, bottom=d1+d2+d3, color='cyan')

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