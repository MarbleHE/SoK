from pathlib import PurePosixPath
from urllib.parse import urlparse
import pandas as pd
import plot_cardio
import plot_nn
from s3_utils import get_most_recent_folder_from_s3_bucket, get_folder_in_s3_path, get_csv_files_in_s3_path, \
    upload_file_to_s3_bucket

BUCKET_NAME = 'sok-repository-eval-benchmarks'


def plot_all_cardio():
    # get the folder with the most recent timestamp (e.g., 20200729_0949/)
    root_folder = get_most_recent_folder_from_s3_bucket()
    # get all subfolders - this corresponds to the benchmarked tools (e.g., Cingulata, SEAL)
    tool_folders_path = get_folder_in_s3_path(root_folder)
    labels = []
    data = []
    for tp in tool_folders_path:
        # get path to all CSV files that have 'cardio' in its name
        name_filter = 'cardio'
        s3_urls = get_csv_files_in_s3_path(tp, name_filter, get_s3_url=True)
        # throw an error if there is more than one CSV with 'cardio' in the folder as each benchmark run should exactly
        # produce one CSV file per program
        if len(s3_urls) == 0:
            # just skip any existing folder without a CSV file (e.g., the plot/ folder)
            continue
        elif len(s3_urls) > 1:
            raise ValueError(
                f"Error: More than one CSV file for '{name_filter}'' found!\nCreate a separate folder for each tool configuration, e.g., SEAL-BFV, SEAL-CKKS.")
        # remove the directory (timestamped folder) segment from the tool's path
        tool_name = tp.replace(root_folder, "")
        # remove the trailing '/' from the tool's name (as it is a directory)
        if tool_name[-1] == '/':
            tool_name = tool_name[:-1]
        # use the tool's name as label for the plot
        labels.append(tool_name)
        # read the CSV data from S3
        data.append(pd.read_csv(s3_urls[0]))

    # sort the data (and labels) according to the following order
    tool__plot_position = [
        'Cingulata-UNOPT',
        'Cingulata',
        'Cingulata-MultiStart',
        'Cingulata-MultiStart-OPT-PARAMS',
        'Lobster',
        'Lobster-OPT-PARAMS',
        'TFHE',
        'SEAL-BFV',
        'SEAL-BFV-Batched',
        'SEAL-CKKS-Batched',
    ]
    labeled_data = dict(zip(labels, data))
    res = {key: labeled_data[key] for key in tool__plot_position}
    labels = list(res.keys())
    data = list(res.values())

    # call the plot
    if len(labels) == 0:
        import sys
        sys.stderr.write("ERROR: Plotting cardio failed because no data is available!")
        return
    fig = plot_cardio.plot(labels, data)
    fig.show()

    # save plot as PDF and PNG in S3
    filename = 'plot_cardio'
    filetypes = ['pdf', 'png']
    for fn, ext in zip(filename, filetypes):
        full_filename = f"{filename}.{ext}"
        fig.savefig(full_filename, format=ext)
        dst_path_s3 = str(PurePosixPath(urlparse(root_folder).path) / 'plot' / full_filename)
        upload_file_to_s3_bucket(full_filename, dst_path_s3)


def plot_all_nn():
    # get the folder with the most recent timestamp (e.g., 20200729_0949/)
    root_folder = get_most_recent_folder_from_s3_bucket()
    # get all subfolders - this corresponds to the benchmarked tools (e.g., Cingulata, SEAL)
    tool_folders_path = get_folder_in_s3_path(root_folder)
    labels = []
    data = []
    for tp in tool_folders_path:
        # get path to all CSV files that have 'nn' in its name
        name_filter = 'nn'
        s3_urls = get_csv_files_in_s3_path(tp, name_filter, get_s3_url=True)
        # throw an error if there is more than one CSV with 'cardio' in the folder as each benchmark run should exactly
        # produce one CSV file per program
        if len(s3_urls) == 0:
            # just skip any existing folder without a CSV file (e.g., the plot/ folder)
            continue
        elif len(s3_urls) > 1:
            raise ValueError(
                f"Error: More than one CSV file for '{name_filter}'' found!\nCreate a separate folder for each tool configuration, e.g., SEAL-BFV, SEAL-CKKS.")
        # remove the directory (timestamped folder) segment from the tool's path
        tool_name = tp.replace(root_folder, "")
        # remove the trailing '/' from the tool's name (as it is a directory)
        if tool_name[-1] == '/':
            tool_name = tool_name[:-1]
        # use the tool's name as label for the plot
        labels.append(tool_name)
        # read the CSV data from S3
        data.append(pd.read_csv(s3_urls[0]))

    # call the plot
    if len(labels) == 0:
        import sys
        sys.stderr.write("ERROR: Plotting neural networks failed because no data is available!")
        return

    fig = plot_nn.plot(labels, data)
    fig.show()

    # save plot as PDF and PNG in S3
    filename = 'plot_nn'
    filetypes = ['pdf', 'png']
    for fn, ext in zip(filename, filetypes):
        full_filename = f"{filename}.{ext}"
        fig.savefig(full_filename, format=ext)
        dst_path_s3 = str(PurePosixPath(urlparse(root_folder).path) / 'plot' / full_filename)
        upload_file_to_s3_bucket(full_filename, dst_path_s3)


def plot_all():
    plot_all_cardio()
    plot_all_nn()


if __name__ == "__main__":
    plot_all()
