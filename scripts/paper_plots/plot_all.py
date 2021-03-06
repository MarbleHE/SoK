import code
from multiprocessing import Process
import plot_cardio
import plot_chi_squared
import plot_kernel
import plot_microbenchmarks
import plot_nn
from pathlib import PurePosixPath
from urllib.parse import urlparse
from matplotlib import pyplot as plt
from s3_utils import upload_file_to_s3_bucket, get_labels_data_from_s3

BUCKET_NAME = 'sok-repository-eval-benchmarks'

output_filetypes = ['pdf', 'png']


def save_plot_in_s3(fig: plt.Figure, filename: str, root_folder: str, use_tight_layout: bool = True):
    for fn, ext in zip(filename, output_filetypes):
        full_filename = f"{filename}.{ext}"

        bbox_inches = 'tight' if (use_tight_layout and ext == 'pdf') else None
        pad_inches = 0 if (use_tight_layout and ext == 'pdf') else 0.1
        dpi = 300
        fig.savefig(full_filename, format=ext, bbox_inches=bbox_inches, pad_inches=pad_inches, dpi=dpi)

        dst_path_s3 = str(PurePosixPath(urlparse(root_folder).path) / 'plot' / full_filename)
        upload_file_to_s3_bucket(full_filename, dst_path_s3)


def plot_all_cardio():
    print("Plotting Cardio")
    try:
        labels, data, root_folder = get_labels_data_from_s3('cardio')
    except TypeError:
        return

    # sort the data (and labels) according to the following order
    tool__plot_position = [
        'Lobster-Baseline',
        'Lobster-Baseline-OPT',
        'MultiStart-OPT-PARAMS',
        'Lobster-OPT-PARAMS',

        'Cingulata-OPT',

        'SEAL-BFV-Manualparams',
        'SEAL-BFV-Naive-Sealparams',
        'E3-SEAL',

        'TFHE',
        'E3-TFHE',
        'Cingulata-TFHE',

        'SEAL-BFV-Batched-Manualparams',
        'E3-SEAL-Batched',
    ]

    labeled_data = dict(zip(labels, data))
    res = {}
    for key in tool__plot_position:
        if key not in labeled_data:
            print(f"Key {key} not found in labels. Skipping it.")
        else:
            res[key] = labeled_data[key]
    labels = list(res.keys())
    data = list(res.values())

    fig = plot_cardio.plot(labels, data)
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_cardio', root_folder)


def plot_all_nn():
    print("Plotting NN")
    try:
        labels, data, root_folder = get_labels_data_from_s3('nn')
    except TypeError:
        return

    fig = plot_nn.plot(labels, data)
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_nn', root_folder, use_tight_layout=True)


def plot_all_chi_squared():
    print("Plotting Chi Squared")
    try:
        labels, data, root_folder = get_labels_data_from_s3('chi_squared')
    except TypeError:
        return

    fig = plot_chi_squared.plot(labels, data)
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_chi_squared', root_folder)


def plot_all_kernel():
    print("Plotting Kernel")
    try:
        labels, data, root_folder = get_labels_data_from_s3('kernel')
    except TypeError:
        return

    fig = plot_kernel.plot(labels, data)
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_kernel', root_folder)


def plot_all_microbenchmark():
    print("Plotting Microbenchmarks")
    try:
        labels, data, root_folder = get_labels_data_from_s3('microbenchmark')
    except TypeError:
        return

    fig = plot_microbenchmarks.plot(labels, data)
    fig.show()

    # save plot in S3
    save_plot_in_s3(fig, 'plot_microbenchmark', root_folder)


def run_in_parallel(*fns):
    proc = []
    for fn in fns:
        p = Process(target=fn)
        p.start()
        proc.append(p)
    for p in proc:
        p.join()


def plot_all():
    # run_in_parallel(plot_all_microbenchmark, plot_all_cardio, plot_all_kernel, plot_all_nn, plot_all_chi_squared)
    plot_all_microbenchmark()
    plot_all_cardio()
    plot_all_nn()
    plot_all_chi_squared()
    plot_all_kernel()


if __name__ == "__main__":
    plot_all()
