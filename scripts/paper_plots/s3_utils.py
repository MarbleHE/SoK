import re
import boto3
import pandas as pd
from botocore.exceptions import ClientError
from typing import List

BUCKET_NAME = 'sok-repository-eval-benchmarks'
client = boto3.client("s3")


def get_csv_files_in_s3_path(path: str, filter: str, get_s3_url: bool = False):
    response = client.list_objects(Bucket=BUCKET_NAME, Prefix=path)
    files = []
    for content in response.get('Contents', []):
        filename = content.get('Key')
        if filter in filename and '.csv' in filename:
            if get_s3_url:
                files.append(f"s3://{BUCKET_NAME}/{filename}")
            else:
                files.append(filename)
    return files


def get_folder_in_s3_path(path: str):
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
    date_pattern = re.compile(r"[0-9_]+")
    for o in result.get('CommonPrefixes'):
        folder_name = o.get('Prefix')
        if re.match(date_pattern, folder_name):
            folders.append(folder_name)
    folders.sort(reverse=True)
    return folders[0]


def upload_file_to_s3_bucket(file_path: str, destination_path: str):
    try:
        client.upload_file(file_path, BUCKET_NAME, destination_path)
    except ClientError as e:
        import sys
        print(f"Could not upload {file_path} to {destination_path} in bucket {BUCKET_NAME}.", file=sys.stderr)


def get_labels_data_from_s3(name_filter: str, root_folder: str = None) -> (List[str], List[pd.DataFrame]):
    labels, data = [], []
    # get the folder with the most recent timestamp (e.g., 20200729_0949/)
    if root_folder is None:
        root_folder = get_most_recent_folder_from_s3_bucket()
    elif not root_folder.endswith('/'):
        root_folder += '/'

    # get all subfolders - this corresponds to the benchmarked tools (e.g., Cingulata, SEAL)
    tool_folders_path = get_folder_in_s3_path(root_folder)
    for tp in tool_folders_path:
        # get path to all CSV files that have 'name_filter' in its name
        s3_urls = get_csv_files_in_s3_path(tp, name_filter, get_s3_url=True)
        # throw an error if there is more than one CSV with 'cardio' in the folder as each benchmark run should exactly
        # produce one CSV file per program
        if len(s3_urls) == 0:
            # just skip any existing folder without a CSV file (e.g., the plot/ folder)
            continue
        elif len(s3_urls) > 1:
            raise ValueError(
                f"Error: More than one CSV file for '{name_filter}'' found!\nCreate a separate folder for each tool "
                f"configuration, e.g., SEAL-BFV, SEAL-CKKS.")
        # remove the directory (timestamped folder) segment from the tool's path
        tool_name = tp.replace(root_folder, "")
        # remove the trailing '/' from the tool's name (as it is a directory)
        if tool_name[-1] == '/':
            tool_name = tool_name[:-1]
        # use the tool's name as label for the plot
        labels.append(tool_name)
        # read the CSV data from S3
        data.append(pd.read_csv(s3_urls[0], delimiter=','))

    # call the plot
    if len(labels) == 0:
        import sys
        sys.stderr.write(f"ERROR: Plotting {name_filter} failed because no data is available!")
        return

    return labels, data, root_folder
