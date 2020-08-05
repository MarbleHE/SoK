import pandas as pd
import matplotlib.pyplot as plt
import boto3
import re
import numpy as np
from typing import List

from s3_utils import get_most_recent_folder_from_s3_bucket, get_folder_in_s3_path, get_csv_files_in_s3_path


# def draw_plot_cardio():
#     # get the folder with the most recent timestamp (e.g., 20200729_0949/)
#     root_folder = get_most_recent_folder_from_s3_bucket()
#
#     # get all subfolders - this corresponds to the benchmarked tools (e.g., Cingulata, SEAL)
#     # e.g., 20200729_0949/Cingulata/
#     tool_folders_path = get_folder_in_s3_path(root_folder)
#
#     # get path to all CSV files that have 'cardio' in its name
#     csv_files = []
#     for tp in tool_folders_path:
#         filename = get_files_in_s3_path(tp, 'cardio')
#         csv_files += filename
#
#     # collect CSV data to generate single plot
#     def read_csv(path_to_csv_files):
#         data = []
#         for s3_path in path_to_csv_files:
#             # for now, skip the "num_conditions" column as we only use the original cardio example program
#             data.append(pd.read_csv(f"s3://{BUCKET_NAME}/{s3_path}",
#                                     sep=',', skipinitialspace=True, usecols=range(1, 5)))
#         return data
#
#     df = pd.concat(read_csv(csv_files), axis=0)
#     print(df)
#     if df.size == 0:
#         return "<div class='error'>There was an error while retrieving the data from S3.</div>"
#
#     # TODO Add code for actual plotting
#     # plot cardio files
#     df.plot()
#     plt.show()
#
#
# def draw_fig():
#     draw_plot_cardio()
#
#
# if __name__ == "__main__":
#     draw_fig()


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
    num_labels = len(labels)
    plt.title('Runtime for Cardio')
    plt.ylabel('Time (ms)')
    ind = np.arange(num_labels)  # the x locations for the groups
    plt.xticks(ind, labels)
    width = 0.35  # the width of the bars: can also be len(x) sequence

    # Plot Bars
    for i in range(num_labels):
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
    plt.show()