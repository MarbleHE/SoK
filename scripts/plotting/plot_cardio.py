import pandas as pd
import matplotlib.pyplot as plt
from threading import Lock
import boto3
import re
import numpy as np

BUCKET_NAME = 'sok-repository-eval-benchmarks'


def draw_plot_cardio():
    # get the folder with the most recent timestamp (e.g., 20200729_0949/)
    root_folder = get_most_recent_folder_from_s3_bucket()

    # get all subfolders - this corresponds to the benchmarked tools (e.g., Cingulata, SEAL)
    # e.g., 20200729_0949/Cingulata/
    tool_folders_path = get_folder_in_s3_path(root_folder)

    # get path to all CSV files that have 'cardio' in its name
    csv_files = []
    for tp in tool_folders_path:
        filename = get_files_in_s3_path(tp, 'cardio')
        csv_files += filename

    # collect CSV data to generate single plot
    def read_csv(path_to_csv_files):
        data = []
        for s3_path in path_to_csv_files:
            # for now, skip the "num_conditions" column as we only use the original cardio example program
            data.append(pd.read_csv(f"s3://{BUCKET_NAME}/{s3_path}",
                                    sep=',', skipinitialspace=True, usecols=range(1, 5)))
        return data

    df = pd.concat(read_csv(csv_files), axis=0)
    print(df)
    if df.size == 0:
        return "<div class='error'>There was an error while retrieving the data from S3.</div>"

    # TODO Add code for actual plotting
    # plot cardio files
    df.plot()
    plt.show()


def draw_fig():
    draw_plot_cardio()


def get_files_in_s3_path(path: str, filter: str):
    client = boto3.client("s3")
    response = client.list_objects(Bucket=BUCKET_NAME, Prefix=path)
    files = []
    for content in response.get('Contents', []):
        filename = content.get('Key')
        if filter in filename:
            files.append(filename)
    return files


def get_folder_in_s3_path(path: str):
    client = boto3.client("s3")
    result = client.list_objects_v2(Bucket=BUCKET_NAME, Prefix=path, Delimiter='/')
    folder_names = []
    for o in result.get('CommonPrefixes'):
        folder_names.append(o.get('Prefix'))
    return folder_names


def get_most_recent_folder_from_s3_bucket():
    """
    Determines the name of the S3 folder that has the most recent timestamp in its name.
    The timestamp is formatted as YYYYMMDD_HHMMSS.
    :return: A string of the most recent folder based on its name (a timestamp).
    """
    s3 = boto3.resource('s3')
    bucket = s3.Bucket(BUCKET_NAME)
    result = bucket.meta.client.list_objects(Bucket=bucket.name, Delimiter='/')
    folders = []
    date_pattern = re.compile(r'[0-9_]+')
    for o in result.get('CommonPrefixes'):
        folder_name = o.get('Prefix')
        if re.match(date_pattern, folder_name):
            folders.append(folder_name)
    folders.sort(reverse=True)
    return folders[0]


if __name__ == "__main__":
    draw_fig()
