import re
from pathlib import PosixPath
from urllib.parse import urlparse

import boto3
from botocore.exceptions import ClientError

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
    date_pattern = re.compile(r'[0-9_]+')
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
